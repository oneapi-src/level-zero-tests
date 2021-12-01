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

*GivenValidFirmwareHandleWhenFlashingFirmwareThenExpectFirmwareFlashingSuccess:- Test checks if zesSysmanFirmwareFlash() API functions properly. The firmware binary should present via the env. variable ZE_LZT_FIRMWARE_DIRECTORY that points to a valid directory that will hold the firmware binary with the same name as expected by the API and exposed via the Firmware properties. This test expects firmware to be flashed without any failure. The firmware binary name present in should match to the firmware types present in the system. Ex: ~/firmwareName.bin. in case the ZE_LZT_FIRMWARE_DIRECTORY is not set or the path pointed by ZE_LZT_FIRMWARE_DIRECTORY does not have a valid firmware binary the test will be skipped with a warning.
