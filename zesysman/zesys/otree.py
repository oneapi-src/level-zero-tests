#!/usr/bin/env python3
# Copyright (C) 2020-2023 Intel Corporation
# SPDX-License-Identifier: MIT

import time

from . import logger
from . import state

pr = logger.pr

RenameAttributes = [ "Name", "Id" ]
IndexAttributes = [ "Index", "UUID" ]
DecorateAttributes = ["Units"]

class Node:
    def __init__(self, parent, name, text, *args, **kwargs):
        self.parent = parent
        self.text = text
        self.name = name
        self.attrs = args
        self.children = []
        self.discard = set(kwargs.get("discard", []))
        self.index = kwargs.get("index", False)
        self.heading = kwargs.get("heading")
        self.join = kwargs.get("join")
        self.rangejoin = kwargs.get("rangejoin", " - ")
        self.width = kwargs.get("width")
        if parent and not kwargs.get("transient"):
            parent.children.append(self)
        headingAttrs = kwargs.get("headingAttrs")
        if headingAttrs is not None:
            self.headingAttrs = [(a,v) for a,v in self.attrs if a in set(headingAttrs)]
        else:
            if any(a in RenameAttributes for a,v in self.attrs):
                self.headingAttrs = [ (a,v) for a,v in self.attrs if a not in IndexAttributes + DecorateAttributes ]
            else:
                self.headingAttrs = [ (a,v) for a,v in self.attrs if a not in DecorateAttributes ]
        self.setFn = kwargs.get("setFn", lambda x:None)
        self.setFn(self)
    def decoratedText(self):
        text = self.text
        for attr,val in self.attrs:
            if attr in DecorateAttributes and text is not None and type(text) != list:
                text = str(text) + " " + str(val)
        return text
    def outputStart(self):
        pass
    def nodeOutput(self):
        return ""
    def outputTree(self):
        pass
    def outputFinish(self):
        pass
    def replaceUnits(self, units):
        self.attrs = [(a,v) for a,v in self.attrs if a != "Units"]
        self.attrs.append(("Units", units))
    def setText(self, text):
        self.text = text
        self.setFn(self)

class IterationNode(Node):
    def __init__(self, parent, name, text, *args, **kwargs):
        super().__init__(parent, name, text, *args, **kwargs)
        self.iteration = 0
    def iterationOutput(self):
        self.iteration += 1
        if state.hideTimestamp:
            timestamp = "%d" % self.iteration
        else:
            fulltime = time.time()
            ms = int(1000 * (fulltime - int(fulltime)))
            timestamp = time.strftime("%H:%M:%S") + ".%03d" % ms
        return timestamp

Node.IterationNode = IterationNode

class RangeNode(Node):
    pass

Node.RangeNode = RangeNode

class ListNode(Node):
    def dataIndent(self, indent=""):
        if "list" in self.discard:
            lengths = [child.dataIndent(indent) for child in self.children]
        else:
            lengths = [child.dataIndent(indent + state.indentStr) for child in self.children]
            name = self.name
            for a,v in self.attrs:
                if a in RenameAttributes:
                    name = str(v)
            lengths.append(len(indent) + len(name))
        return max(lengths)
    def nodeOutput(self, indent, dataIndent):
        childIdentifiers = []
        nodeText = indent + str(self.name)
        text = self.decoratedText()
        for attr,val in self.attrs:
            if attr in RenameAttributes:
                nodeText = indent + str(val)
            elif attr in IndexAttributes:
                if attr == state.indexAttribute:
                    nodeText += " " + str(val)
                else:
                    childIdentifiers.append(self.__class__(self, attr, val, transient=True))
            elif attr not in DecorateAttributes:
                nodeText += "." + attr + "_" + str(val)
        if text is not None:
            if type(text) == list:
                nodeText += " " * (dataIndent - len(nodeText)) + " : " + ", ".join(text)
            else:
                nodeText += " " * (dataIndent - len(nodeText)) + " : " + str(text)
        return (nodeText, childIdentifiers)
    def outputTree(self, indent="", dataIndent=None):
        if dataIndent is None:
            if state.condensedList:
                dataIndent = 0
            elif "list" in self.discard:
                dataIndent = max([child.dataIndent() for child in self.children])
            else:
                dataIndent = self.dataIndent()
        nodeText, childIdentifiers = self.nodeOutput(indent, dataIndent)
        if "list" in self.discard:
            for child in childIdentifiers + self.children:
                child.outputTree(indent, dataIndent)
        else:
            pr(nodeText)
            for child in childIdentifiers + self.children:
                child.outputTree(indent + state.indentStr, dataIndent)

class IterationListNode(ListNode, IterationNode):
    def __init__(self, parent, name, text, *args, **kwargs):
        IterationNode.__init__(self, parent, name, text, *args, **kwargs)
    def nodeOutput(self, indent, dataIndent):
        timeStr = self.iterationOutput()
        if state.hideTimestamp:
            nodeText = "\n" + indent + "Iteration " + str(self.iteration)
        else:
            nodeText = "\n" + indent + timeStr
        return (nodeText, [])

ListNode.IterationNode = IterationListNode

class RangeListNode(ListNode):
    pass

ListNode.RangeNode = RangeListNode

class XmlNode(Node):
    def outputStart(self):
        pr('<?xml version="1.0" ?>')
    def nodeOutput(self, indent):
        childIdentifiers = []
        nodeStart = indent + "<" + str(self.name)
        for attr,val in self.attrs:
            nodeStart += " " + attr + '="' + str(val) + '"'
        nodeText = None
        nodeFinish = None
        if self.children:
            nodeStart += ">"
            if self.text:
                if type(self.text) == list:
                    nodeText = indent + state.indentStr + ("\n" + indent + state.indentStr).join(self.text)
                else:
                    nodeText = indent + state.indentStr + str(self.text)
            nodeFinish = indent + "</" + str(self.name) + ">"
        elif type(self.text) == list:
            if len(self.text) == 0:
                nodeStart += "/>"
            elif len(self.text) == 1:
                nodeStart += ">" + self.text[0] + "</" + str(self.name) + ">"
            else:
                nodeStart += ">"
                nodeText = indent + state.indentStr + ("\n" + indent + state.indentStr).join(self.text)
                nodeFinish = indent + "</" + str(self.name) + ">"
        elif self.text is not None and self.text != "":
            nodeStart += ">" + str(self.text) + "</" + str(self.name) + ">"
        else:
            nodeStart += "/>"
        return (nodeStart, nodeText, nodeFinish, childIdentifiers)
    def outputTree(self, indent=""):
        nodeStart, nodeText, nodeFinish, childIdentifiers = self.nodeOutput(indent)
        if "xml" in self.discard:
            for child in childIdentifiers + self.children:
                child.outputTree(indent)
        else:
            if nodeStart:
                pr(nodeStart)
            if nodeText:
                pr(nodeText)
            for child in childIdentifiers + self.children:
                child.outputTree(indent + state.indentStr)
            if nodeFinish:
                pr(nodeFinish)

class IterationXmlNode(XmlNode, IterationNode):
    def __init__(self, parent, name, text, *args, **kwargs):
        IterationNode.__init__(self, parent, name, text, *args, **kwargs)
    def nodeOutput(self, indent):
        timeStr = self.iterationOutput()
        nodeStart = '<Iteration Num="%d">' % self.iteration
        nodeText = None
        nodeFinish = '</Iteration>'
        childIdentifiers = []
        if not state.hideTimestamp:
            childIdentifiers.append(XmlNode(self, "Timestamp", timeStr, transient=True))
        return (nodeStart, nodeText, nodeFinish, childIdentifiers)

XmlNode.IterationNode = IterationXmlNode

class RangeXmlNode(XmlNode):
    pass

XmlNode.RangeNode = RangeXmlNode

class TableNode(Node):
    def __init__(self, parent, name, text, *args, **kwargs):
        # pr(parent and parent.name, "->", name, text, args, kwargs)
        super().__init__(parent, name, text, *args, **kwargs)
        if self.width is None:
            if self.index or text is not None:
                self.width = max(len(self.headerString()), len(self.nodeOutput()))
            else:
                self.width = 0
    def discarded(self):
        return "table" in self.discard
    def headerString(self):
        if self.join is not None:
            prefix = self.parent.headerString() + self.join
        else:
            prefix = ""
        if self.heading is not None:
            name = self.heading
        elif self.index:
            return prefix + state.indexAttribute
        elif self.discarded() or self.text is None and not self.headingAttrs:
            return prefix
        else:
            name = str(self.name)
        for attr, val in self.headingAttrs:
            name += "[%s]" % str(val)
        return prefix + name
    def headerList(self):
        myHeader = []
        if self.width != 0:
            myHeader.append((self.width, self.headerString()))
        childHeaders = []
        for child in self.children:
            if child.index:
                childHeaders.append(child.headerList())
            else:
                childHeaders += child.headerList()
        return myHeader + childHeaders
    def flatHeaderList(self, pad):
        leftHeaders, allIndexed, rightHeaders = [], [], []
        indexedStarted, indexedFinished = False, False
        allHeaders = self.headerList()
        for header in allHeaders:
            if not indexedStarted:
                if type(header) == tuple:
                    width, text = header
                    if pad:
                        text += " " * (width - len(text))
                    leftHeaders.append(text)
                elif type(header) == list:
                    indexedStarted = True
                else:
                    pr.fail("Internal error - bad header list")

            if indexedStarted and not indexedFinished:
                if type(header) == list:
                    allIndexed.append(header)
                    for nested in header:
                        if type(nested) != tuple:
                            pr.fail("Internal error - doubly-nested header list")
                elif type(header) == tuple:
                    indexedFinished = True
                else:
                    pr.fail("Internal error - bad header list")

            if indexedFinished:
                if type(header) == tuple:
                    width, text = header
                    if pad:
                        text += " " * (width - len(text))
                    rightHeaders.append(text)
                else:
                    pr.fail("Internal error - multiply-indexed header list")

        if not allIndexed:
            return leftHeaders + rightHeaders

        headerNames = [n for _,n in allIndexed[0]]
        for row in range(1, len(allIndexed)):
            headerRow = allIndexed[row].copy()
            headerCol = 0
            for col in range(0, len(headerRow)):
                width, name = headerRow[col]
                if headerCol >= len(headerNames):
                    headerNames.append(name)
                    for r in range(row):
                        allIndexed[r].append((0, name))
                elif name == headerNames[headerCol]:
                    headerCol += 1
                else:
                    remainingNames = headerNames[headerCol:]
                    if name in remainingNames:
                        skipCount = remainingNames.index(name)
                        allIndexed[row][headerCol:headerCol] = [(0, n) for n in remainingNames[:skipCount]]
                        headerCol += skipCount + 1
                    else:
                        headerNames.insert(headerCol,name)
                        for r in range(row):
                            allIndexed[r].insert(headerCol,(0, name))
                        headerCol += 1

        indexWidths = []
        for headerRow in allIndexed:
            indexWidths.append([width for width, _ in headerRow])

        self.indexWidths = indexWidths
        widths = [max(w) for w in zip(*indexWidths)]

        indexedHeaders = []
        for width, text in zip(widths, headerNames):
            if pad:
                text += " " * (width - len(text))
            indexedHeaders.append(text)

        return leftHeaders + indexedHeaders + rightHeaders
    def outputStart(self):
        headers = self.flatHeaderList(pad=True)
        pr("+-" + "-+-".join(["-" * len(h) for h in headers]) + "-+")
        pr("| " + " | ".join(headers) + " |")
        pr("|=" + "=|=".join(["=" * len(h) for h in headers]) + "=|")
    def nodeOutput(self):
        if self.index:
            return str([v for a,v in self.attrs if a == state.indexAttribute][0])
        elif type(self.text) == list:
            return ", ".join(self.text)
        else:
            return str(self.decoratedText())
    def rowList(self):
        myCell = []
        if self.width != 0:
            myCell.append((self.width, self.nodeOutput()))
        childRows = []
        for child in self.children:
            if child.index:
                childRows.append(child.rowList())
            else:
                childRows += child.rowList()
        return myCell + childRows
    def flatRowList(self, pad):
        leftRows, allIndexed, rightRows = [], [], []
        indexedStarted, indexedFinished = False, False
        for row in self.rowList():
            if not indexedStarted:
                if type(row) == tuple:
                    width, text = row
                    if pad:
                        text += " " * (width - len(text))
                    leftRows.append(text)
                elif type(row) == list:
                    indexedStarted = True
                else:
                    pr.fail("Internal error - bad row list")

            if indexedStarted and not indexedFinished:
                if type(row) == list:
                    allIndexed.append(row)
                    for nested in row:
                        if type(nested) != tuple:
                            pr.fail("Internal error - doubly-nested row list")
                elif type(row) == tuple:
                    indexedFinished = True
                else:
                    pr.fail("Internal error - bad row list")

            if indexedFinished:
                if type(row) == tuple:
                    width, text = row
                    if pad:
                        text += " " * (width - len(text))
                    rightRows.append(text)
                else:
                    pr.fail("Internal error - multiply-indexed row list")

        allWidths = []
        allText = []
        for row in range(len(allIndexed)):
            indexWidths = self.indexWidths[row]
            widths = []
            texts = []
            indexedRow = allIndexed[row]
            indexedCol = 0
            for col in range(len(indexWidths)):
                if indexWidths[col]:
                    width, text = indexedRow[indexedCol]
                    widths.append(max(width, indexWidths[col]))
                    texts.append(text)
                    indexedCol += 1
                else:
                    widths.append(0)
                    texts.append("")
            allWidths.append(widths)
            allText.append(texts)

        widths = [max(w) for w in zip(*allWidths)]
        if not allText:
            allText = [[]]

        rows = []

        for row in allText:
            indexedRows = []
            for width, text in zip(widths, row):
                if pad:
                    if text == "":
                        text = " " * (width//2) + "-"
                        text += " " * (width - len(text))
                    else:
                        text += " " * (width - len(text))
                indexedRows.append(text)
            rows.append(leftRows + indexedRows + rightRows)

        return rows
    def outputTree(self):
        for row in self.flatRowList(pad=True):
            pr("| " + " | ".join(row) + " |")
    def outputFinish(self):
        headers = self.flatHeaderList(pad=True)
        pr("+-" + "-+-".join(["-" * len(h) for h in headers]) + "-+")
    def setText(self, text):
        super().setText(text)
        self.width = max(len(self.nodeOutput()), self.width)

class IterationTableNode(TableNode, IterationNode):
    def __init__(self, parent, name, text, *args, **kwargs):
        IterationNode.__init__(self, parent, name, text, *args, **kwargs)
        if state.hideTimestamp:
            self.width = len("Iteration")
        else:
            self.width = len("HH:MM:SS.xxx")
    def headerString(self):
        if state.hideTimestamp:
            return "Iteration"
        else:
            return "Time"
    def nodeOutput(self):
        return self.iterationOutput()

TableNode.IterationNode = IterationTableNode

class RangeTableNode(TableNode):
    def headerString(self):
        if self.join is not None:
            prefix = self.parent.headerString() + self.join
        else:
            prefix = ""
        if self.heading is not None:
            name = self.heading
        elif self.discarded():
            return prefix
        else:
            name = str(self.name)
        for attr, val in self.headingAttrs:
            name += "[%s]" % str(val)
        return prefix + name
    def headerList(self):
        self.width = max(len(self.nodeOutput()), len(self.headerString()))
        return [(self.width, self.headerString())]
    def nodeOutput(self):
        text = ""
        for child in self.children:
            if text:
                text += child.rangejoin
            text += str(child.decoratedText())
        return text
    def rowList(self):
        return [(self.width, self.nodeOutput())]

TableNode.RangeNode = RangeTableNode

def csvQuote(s):
    s = s.replace('"','""')
    if '"' in s or "," in s or "\n" in s:
        return '"' + s + '"'
    else:
        return s

class CsvNode(TableNode):
    def discarded(self):
        return "csv" in self.discard
    def outputStart(self):
        pr(",".join([ csvQuote(c) for c in self.flatHeaderList(pad=False) ]))
    def outputTree(self):
        for row in self.flatRowList(pad=False):
            pr(",".join([ csvQuote(c) for c in row ]))
    def outputFinish(self):
        pass

# Currently-selected Node class
NodeClass = ListNode

def setNodeClassByName(name):
    global NodeClass
    classMap = { "list" : ListNode, "xml" : XmlNode,
                 "table" : TableNode, "csv" : CsvNode }

    NodeClass = classMap.get(name, NodeClass)

def setNodeClassByExtension(ext):
    global NodeClass
    classMap = { ".xml" : XmlNode, ".csv" : CsvNode }

    NodeClass = classMap.get(ext, NodeClass)

def setNullNodeClass():
    global NodeClass
    NodeClass = Node
