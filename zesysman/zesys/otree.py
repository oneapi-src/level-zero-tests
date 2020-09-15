#!/usr/bin/env python3
# Copyright (C) 2020 Intel Corporation
# SPDX-License-Identifier: MIT

import xml.etree.ElementTree as ElementTree
import xml.dom.minidom as minidom

from . import logger
from . import state

IgnoreAttributes = [ "Number" ]
RenameAttributes = [ "Name", "Id" ]
IndexAttributes = [ "Index" ]
SecondaryIndexAttributes = [ "UUID" ]
DecorateAttributes = ["Units"]
TableRenameNodes = { "TemperatureSensor" : "Temp",
                     "PowerDomain" : "Power",
                     "FrequencyDomain" : "Freq",
                     "RequestedFrequency" : "Requested",
                     "ActualFrequency" : "Actual",
                     "MemoryModule" : "Mem",
                     "ErrorDomain" : "Err",
                     "FabricPort" : "Port",
                     "StandbyDomain" : "Standby",
                     "EngineGroup" : "",
                     "Activity" : "" }
TablePostRename = { "AllEngines" : "Util",
                    "AllComputeEngines" : "AllComputeUtil",
                    "AllMediaEngines" : "AllMediaUtil",
                    "AllCopyEngines" : "AllCopyUtil",
                    "ComputeEngine" : "ComputeUtil",
                    "RenderEngine" : "RenderUtil",
                    "MediaDecodeEngine" : "DecodeUtil",
                    "MediaEncodeEngine" : "EncodeUtil",
                    "CopyEngine" : "CopyUtil" }
TableKeepNodes = [ "PCI" ]

def tableNode(parent, node, text, *attrs):
    if parent is None:
        if state.maxIterations > 1:
            return (["Iteration"], [])
        else:
            return ([], [])
    elif len(parent) == 2:
        columns, rows = parent
        index = len(rows)
        rows.append({})

        for attr,val in attrs:
            if attr in IndexAttributes:
                if attr not in columns:
                    columns.append(attr)
                rows[index][attr] = str(val)
        return (columns, rows, index, "")
    else:
        node = TableRenameNodes.get(node, str(node))
        attrs = [ (a,v) for a,v in attrs if a not in IgnoreAttributes ]
        if text is None and node not in TableKeepNodes and not attrs:
            return parent
        if any(a in RenameAttributes for a,v in attrs):
            attrs = [ (a,v) for a,v in attrs if a not in IndexAttributes ]
        columns, rows, index, column = parent
        for attr,val in attrs:
            if node or column:
                node += "[" + str(val) + "]"
            else:
                node = str(val)
        node = TablePostRename.get(node, str(node))
        if column and node and node[0] != "[":
            column += "."
        column += node
        if text is not None:
            if column not in columns:
                columns.append(column)
            rows[index][column] = str(text)
        return (columns, rows, index, column)

def makeTableLine(chEnd, chLine, widths):
    return chEnd + chEnd.join([chLine * (w+2) for w in widths]) + chEnd

tableEndLine = ""

def tableOutputNode(node, currentIteration):
    global tableEndLine
    columns, rows = node
    widths = [ max([len(w)] + [len(r.get(w,"")) for r in rows]) for w in columns ]
    if currentIteration < 2:
        tableEndLine = makeTableLine("+", "-", widths)
        headings = [ " " + c + " " * (w - len(c)) + " " for c,w in zip(columns, widths) ]
        innerLine = makeTableLine("|", "=", widths)
        logger.pr(tableEndLine)
        logger.pr("|" + "|".join(headings) + "|")
        logger.pr(innerLine)
    for row in rows:
        line = []
        if state.maxIterations > 1:
            row["Iteration"] = str(currentIteration)
        for column,width in zip(columns,widths):
            text = row.get(column, " " * (width // 2) + "-")
            line.append(" " + text + " " * (width - len(text)) + " ")
        logger.pr("|" + "|".join(line) + "|")

def tableOutputComplete():
    global tableEndLine
    logger.pr(tableEndLine)

def tableSetText(node, text, *attrs):
    if len(node) == 4:
        columns, rows, index, column = node
        rows[index][column] = str(text)

def csvQuote(s):
    s = s.replace('"','""')
    if '"' in s or "," in s or "\n" in s:
        return '"' + s + '"'
    else:
        return s

def csvOutputNode(node, currentIteration):
    columns, rows = node
    if currentIteration < 2:
        logger.pr(",".join([ csvQuote(c) for c in columns ]))
    for row in rows:
        line = []
        if state.maxIterations > 1:
            row["Iteration"] = str(currentIteration)

        for column in columns:
            line.append(csvQuote(row.get(column,"")))
        logger.pr(",".join(line))

def listNode(parent, node, text, *attrs):
    if parent is None:
        if state.maxIterations > 1:
            children = [ state.indentStr ]
        else:
            children = [ "" ]
    else:
        indent = parent[0]
        n = indent + str(node)
        children = [ indent + state.indentStr ]
        attrs = [ (a,v) for a,v in attrs if a not in IgnoreAttributes ]
        for attr,val in attrs:
            if attr in RenameAttributes:
                n = indent + str(val)
            elif attr in IndexAttributes:
                n += " " + str(val)
            elif attr in SecondaryIndexAttributes:
                listNode(children, attr, val)
            elif attr in DecorateAttributes and text is not None:
                text = str(text) + " " + str(val)
            else:
                n += "." + attr + "_" + str(val)

        if text is None:
            parent.append(([n], children))
        else:
            leaf_node = [n, str(text)]
            parent.append((leaf_node, children))
            # this only works because all text nodes are leaf nodes:
            return leaf_node

    return children

def listDataIndent(i, node):
    for child, descendents in node[1:]:
        i = max(len(child[0]), i, listDataIndent(i, descendents))
    return i

def prListIndented(i, node):
    for child, descendents in node[1:]:
        if len(child) == 1:
            logger.pr(child[0])
        else:
            logger.pr(child[0] + " " * (i - len(child[0])), ":", " ".join(child[1:]))
        prListIndented(i, descendents)

def listOutputNode(node, currentIteration):
    if state.maxIterations > 1:
        logger.pr("\nIteration", currentIteration)
    if state.condensedList:
        prListIndented(0, node)
    else:
        prListIndented(listDataIndent(0, node), node)

def listSetText(node, text, *attrs):
    if len(node) == 2:
        text = str(text)
        for attr,val in attrs:
            if attr in DecorateAttributes:
                text += " " + str(val)
        node[1] = text

xmlIterationCount = None

def xmlNode(parent, node, text, *attrs):
    global xmlIterationCount
    if parent is None:
        if state.maxIterations > 1:
            if xmlIterationCount is None:
                xmlIterationCount = ElementTree.Element("Iteration")
                xmlIterationCount.set("Num", "?")
            e = ElementTree.SubElement(xmlIterationCount, node)
        else:
            new_elem = ElementTree.Element(node)
            e = new_elem
    else:
        new_elem = ElementTree.SubElement(parent, node)
        e = new_elem
    for attr, value in attrs:
        e.set(attr, str(value))
    if text is not None:
        e.text = str(text)
    return e

def xmlPrettyPrint(node):
    xml = minidom.parseString(ElementTree.tostring(node))
    text = xml.toprettyxml(indent=state.indentStr).rstrip()
    return "\n".join(text.split("\n")[1:])

def xmlOutputNode(node, currentIteration):
    global xmlIterationCount
    if currentIteration == 1:
        logger.pr('<?xml version="1.0" ?>')
    if state.maxIterations > 1 and xmlIterationCount is not None:
        xmlIterationCount.set("Num", str(currentIteration))
        node = xmlIterationCount
    text = xmlPrettyPrint(node)
    logger.pr(text)

def xmlOutputComplete():
    global xmlIterationCount
    xmlIterationCount = None

def xmlSetText(node, text, *attrs):
    node.text = str(text)

def noAction(*args, **kwargs):
    pass

#
# Function pointers for each format
#

class FormatNode:
    def __init__(self, parent, name, text, *attrs):
        self.parent = parent
        self.name = name
        self.text = text
        self.attrs = attrs
    def output(self, currentIteration):
        pass
    def outputComplete(self):
        pass
    def setText(self, text):
        self.text = text

class ListFormatNode(FormatNode):
    def __init__(self, parent, name, text, *attrs):
        super().__init__(parent, name, text, *attrs)
        if parent:
            parent = parent.node
        self.node = listNode(parent, name, text, *attrs)
    def output(self, currentIteration):
        listOutputNode(self.node, currentIteration)
    def setText(self, text):
        super().setText(text)
        listSetText(self.node, text, *self.attrs)

class XMLFormatNode(FormatNode):
    def __init__(self, parent, name, text, *attrs):
        super().__init__(parent, name, text, *attrs)
        if parent:
            parent = parent.node
        self.node = xmlNode(parent, name, text, *attrs)
    def output(self, currentIteration):
        xmlOutputNode(self.node, currentIteration)
    def outputComplete(self):
        xmlOutputComplete()
    def setText(self, text):
        super().setText(text)
        xmlSetText(self.node, text, *self.attrs)

class TableFormatNode(FormatNode):
    def __init__(self, parent, name, text, *attrs):
        super().__init__(parent, name, text, *attrs)
        if parent:
            parent = parent.node
        self.node = tableNode(parent, name, text, *attrs)
    def output(self, currentIteration):
        tableOutputNode(self.node, currentIteration)
    def outputComplete(self):
        tableOutputComplete()
    def setText(self, text):
        super().setText(text)
        tableSetText(self.node, text, *self.attrs)

class CSVFormatNode(FormatNode):
    def __init__(self, parent, name, text, *attrs):
        super().__init__(parent, name, text, *attrs)
        if parent:
            parent = parent.node
        self.node = tableNode(parent, name, text, *attrs)
    def output(self, currentIteration):
        csvOutputNode(self.node, currentIteration)
    def setText(self, text):
        super().setText(text)
        tableSetText(self.node, text, *self.attrs)

# Currently-selected Node class
NodeClass = ListFormatNode

def setNodeClassByName(name):
    global NodeClass
    classMap = { "list" : ListFormatNode, "xml" : XMLFormatNode,
                 "table" : TableFormatNode, "csv" : CSVFormatNode }

    NodeClass = classMap.get(name, NodeClass)

def setNodeClassByExtension(ext):
    global NodeClass
    classMap = { ".xml" : XMLFormatNode, ".csv" : CSVFormatNode }

    NodeClass = classMap.get(ext, NodeClass)

def setNullNodeClass():
    global NodeClass
    NodeClass = FormatNode
