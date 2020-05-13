#testSysmanFirmware

## Description

This test suite checks the functionality of the Firmware API's used by sysman. 
Please note that the Firmware API's are sensitive and require root privileges.

###zetSysmanFirmwareGet

The following test validate the zetSysmanFirmwareGet() API

*GivenComponentCountZeroWhenRetrievingFirmwareHandlesThenNotNullFirmwareHandlesAreReturned:- this test checks if the zetSysmanFirmwareGet() API returns success.

###zetSysmanFirmwareGetProperties

The following tests validate the zetSysmanFirmwareGetProperties() API

*GivenValidFirmwareHandleWhenRetrievingFirmwarePropertiesThenValidPropertiesAreReturned:- Test checks whether the zetSysmanFirmwareGetProperties() returns success and the properties are valid.

*GivenValidFirmwareHandleWhenRetrievingFirmwarePropertiesThenExpectSamePropertiesReturnedTwice:- Test checks whether the zetSysmanFirmwareGetProperties() returns success and the properties are valid and equal on subsequent calls.

###zetSysmanFirmwareGetChecksum

The following test validate the zetSysmanFirmwareGetChecksum() API

*GivenValidFirmwareHandleWhenRetrievingFirmwareChecksumThenExpectValidIntegerType:- Test checks if the zetSysmanFirmwareGetChecksum() API returns success and validates the returned checksum.

###zetSysmanFirmwareFlash

The following test validates the zetSysmanFirmwareFlash() API

*GivenValidFirmwareHandleWhenFlashingFirmwareThenExpectUpdatedChecksum:- Test checks if zetSysmanFirmwareFlash() API functions properly by comparing the checksum of the flashed firmware image and the test firmware image. the Firmware itself is provided by setting the env. variable ZE_LZT_FIRMWARE_DIRECTORY to point to a valid directory that will hold the firmware with the same name as expected by the API and exposed via the Firmware properties.THis test expects the checksum of the image present in the directory to save in the format of <fwImageName>.cksum for validation.
