// matth-x/MicroOcpp
// Copyright Matthias Akstaller 2019 - 2024
// MIT License

#ifndef FIRMWARESERVICE_H
#define FIRMWARESERVICE_H

#include <functional>
#include <memory>

#include <MicroOcpp/Core/ConfigurationKeyValue.h>
#include <MicroOcpp/Model/FirmwareManagement/FirmwareStatus.h>
#include <MicroOcpp/Core/Time.h>
#include <MicroOcpp/Core/Ftp.h>

namespace MicroOcpp {

enum class DownloadStatus {
    NotDownloaded, // == before download or during download
    Downloaded,
    DownloadFailed
};

enum class InstallationStatus {
    NotInstalled, // == before installation or during installation
    Installed,
    InstallationFailed
};

class Context;
class Request;

class FirmwareService {
private:
    Context& context;
    
    std::shared_ptr<Configuration> previousBuildNumberString;
    std::string buildNumber;

    std::function<DownloadStatus()> downloadStatusInput;
    bool downloadIssued = false;

    std::shared_ptr<FtpClient> ftpClient;
    std::unique_ptr<FtpDownload> ftpDownload;
    DownloadStatus ftpDownloadStatus = DownloadStatus::NotDownloaded;
    const char *ftpServerCert = nullptr;

    std::function<InstallationStatus()> installationStatusInput;
    bool installationIssued = false;

    Ocpp16::FirmwareStatus lastReportedStatus = Ocpp16::FirmwareStatus::Idle;
    bool checkedSuccessfulFwUpdate = false;

    std::string location;
    Timestamp retreiveDate;
    unsigned int retries = 0;
    unsigned int retryInterval = 0;

    std::function<bool(const char *location)> onDownload;
    std::function<bool(const char *location)> onInstall;

    unsigned long delayTransition = 0;
    unsigned long timestampTransition = 0;

    enum class UpdateStage {
        Idle,
        AwaitDownload,
        Downloading,
        AfterDownload,
        AwaitInstallation,
        Installing,
        Installed,
        InternalError
    } stage = UpdateStage::Idle;

    void resetStage();

    std::unique_ptr<Request> getFirmwareStatusNotification();

public:
    FirmwareService(Context& context);

    void setBuildNumber(const char *buildNumber);

    void loop();

    void scheduleFirmwareUpdate(const char *location, Timestamp retreiveDate, unsigned int retries = 1, unsigned int retryInterval = 0);

    Ocpp16::FirmwareStatus getFirmwareStatus();

    /*
     * Sets the firmware writer. During the UpdateFirmware process, MO will use an FTP client to download the firmware and forward
     * the binary data to `firmwareWriter`. The binary data comes in chunks. MO executes `firmwareWriter` with `buf` containing the
     * next chunk of FW data and `size` being the chunk size. `firmwareWriter` must return the number of bytes written, whereas
     * the result can be between 1 and `size`, and 0 aborts the download. MO executes `onClose` with the reason why the connection
     * has been closed. If the download hasn't been successful, MO will abort the update routine in any case.
     * 
     * Note that this function only works if MO_ENABLE_MBEDTLS=1, or MO has been configured with a custom FTP client
     */
    void setDownloadFileWriter(std::function<size_t(const unsigned char *buf, size_t size)> firmwareWriter, std::function<void(MO_FtpCloseReason)> onClose);

    /*
     * Set an FTP client manually or replace the default FTP client, if built with `MO_ENABLE_MBEDTLS=1`
     */
    void setFtpClient(std::shared_ptr<FtpClient> ftpClient);

    void setFtpServerCert(const char *cert); //zero-copy mode, i.e. cert must outlive MO

    /*
     * Manual alternative for FTP download handler `setDownloadFileWriter`
     */
    void setOnDownload(std::function<bool(const char *location)> onDownload);

    void setDownloadStatusInput(std::function<DownloadStatus()> downloadStatusInput);

    void setOnInstall(std::function<bool(const char *location)> onInstall);

    void setInstallationStatusInput(std::function<InstallationStatus()> installationStatusInput);
};

} //endif namespace MicroOcpp

#if !defined(MO_CUSTOM_UPDATER)

#if MO_PLATFORM == MO_PLATFORM_ARDUINO && defined(ESP32) && MO_ENABLE_MBEDTLS

namespace MicroOcpp {
std::unique_ptr<FirmwareService> makeDefaultFirmwareService(Context& context, std::shared_ptr<FtpClient> ftpClient);
}

#elif MO_PLATFORM == MO_PLATFORM_ARDUINO && defined(ESP8266)

namespace MicroOcpp {
std::unique_ptr<FirmwareService> makeDefaultFirmwareService(Context& context);
}

#endif //MO_PLATFORM
#endif //!defined(MO_CUSTOM_UPDATER)

#endif
