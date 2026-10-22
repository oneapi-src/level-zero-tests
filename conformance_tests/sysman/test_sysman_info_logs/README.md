# test_sysman_info_logs

Sysman info logs extension (`ZES_extension_info_logs`) related test content. Covers `zesDriverEnumInfoLogsExt`, `zesInfoLogGetPropertiesExt`, `zesInfoLogCreateInstanceExt`, `zesInfoLogInstanceReadWithMetadataExt`, `zesInfoLogInstancePeekWithMetadataExt` and `zesInfoLogInstanceDeleteExt`.

Tests are skipped when the driver does not support the extension, reports no info logs, or the user lacks permission to collect info log records.
