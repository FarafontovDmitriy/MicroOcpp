// matth-x/ArduinoOcpp
// Copyright Matthias Akstaller 2019 - 2023
// MIT License

#ifndef CONNECTOR_H
#define CONNECTOR_H

#include <ArduinoOcpp/Tasks/ChargeControl/OcppEvseState.h>
#include <ArduinoOcpp/Core/ConfigurationKeyValue.h>
#include <ArduinoOcpp/Core/PollResult.h>
#include <ArduinoOcpp/MessagesV16/CiStrings.h>

#include <vector>
#include <functional>
#include <memory>

#define AVAILABILITY_OPERATIVE 2
#define AVAILABILITY_INOPERATIVE_SCHEDULED 1
#define AVAILABILITY_INOPERATIVE 0

namespace ArduinoOcpp {

class OcppEngine;
class OcppModel;
class OcppMessage;
class Transaction;

class Connector {
private:
    OcppEngine& context;
    OcppModel& model;
    
    const int connectorId;

    std::shared_ptr<Transaction> transaction;

    std::shared_ptr<Configuration<int>> availability;
    int availabilityVolatile = AVAILABILITY_OPERATIVE;

    std::function<bool()> connectorPluggedSampler;
    std::function<bool()> evRequestsEnergySampler;
    std::function<bool()> connectorEnergizedSampler;
    std::vector<std::function<const char *()>> connectorErrorCodeSamplers;
    const char *getErrorCode();

    OcppEvseState currentStatus = OcppEvseState::NOT_SET;
    std::shared_ptr<Configuration<int>> minimumStatusDuration; //in seconds
    OcppEvseState reportedStatus = OcppEvseState::NOT_SET;
    unsigned long t_statusTransition = 0;

    std::function<PollResult<bool>()> onUnlockConnector;

    std::function<bool()> startTxReadyInput; //the StartTx request will be delayed while this Input is false
    std::function<bool()> stopTxReadyInput; //the StopTx request will be delayed while this Input is false
    std::function<bool()> occupiedInput; //instead of Available, go into Preparing / Finishing state

    std::shared_ptr<Configuration<int>> connectionTimeOut; //in seconds
    std::shared_ptr<Configuration<bool>> stopTransactionOnInvalidId;
    std::shared_ptr<Configuration<bool>> stopTransactionOnEVSideDisconnect;
    std::shared_ptr<Configuration<bool>> unlockConnectorOnEVSideDisconnect;
    std::shared_ptr<Configuration<bool>> localPreAuthorize;
    std::shared_ptr<Configuration<bool>> allowOfflineTxForUnknownId;

    std::shared_ptr<Configuration<bool>> silentOfflineTransactions;
    std::shared_ptr<Configuration<int>> authorizationTimeout; //in seconds
    std::shared_ptr<Configuration<bool>> freeVendActive;
    std::shared_ptr<Configuration<const char*>> freeVendIdTag;
    bool freeVendTrackPlugged = false;
public:
    Connector(OcppEngine& context, int connectorId);
    Connector(Connector&& other) = default;

    /*
     * beginTransaction begins the transaction process which eventually leads to a StartTransaction
     * request in the normal case.
     * 
     * If the transaction process begins successfully, a Transaction object is returned
     * If no transaction process begins due to this call, nullptr is returned (e.g. memory allocation failed)
     */
    std::shared_ptr<Transaction> beginTransaction(const char *idTag);
    std::shared_ptr<Transaction> beginTransaction_authorized(const char *idTag, const char *parentIdTag = nullptr);

    /*
     * End the current transaction process, if existing and not ended yet. This eventually leads to
     * a StopTransaction request, if the transaction process has actually ended due to this call. It
     * is safe to call this function at any time even if no transaction is running
     */
    void endTransaction(const char *reason = nullptr);
    const char *getSessionIdTag();
    uint16_t getSessionWriteCount();
    bool isTransactionRunning();
    int getTransactionId();
    int getTransactionIdSync();
    std::shared_ptr<Transaction>& getTransaction();

    //create detached transaction - won't have any side-effects with the transaction handling of this lib
    std::shared_ptr<Transaction> allocateTransaction(); 

    int getAvailability();
    void setAvailability(bool available);
    void setAvailabilityVolatile(bool available); //set inoperative state but keep only until reboot at most
    void setAuthorizationProvider(std::function<const char *()> authorization);
    void setConnectorPluggedSampler(std::function<bool()> connectorPlugged);
    void setEvRequestsEnergySampler(std::function<bool()> evRequestsEnergy);
    void setConnectorEnergizedSampler(std::function<bool()> connectorEnergized);
    void addConnectorErrorCodeSampler(std::function<const char*()> connectorErrorCode);

    void loop();

    OcppEvseState inferenceStatus();

    bool ocppPermitsCharge();

    void setOnUnlockConnector(std::function<PollResult<bool>()> unlockConnector);
    std::function<PollResult<bool>()> getOnUnlockConnector();

    void setStartTxReadyInput(std::function<bool()> startTxReady);
    void setStopTxReadyInput(std::function<bool()> stopTxReady);
    void setOccupiedInput(std::function<bool()> occupied);
};

} //end namespace ArduinoOcpp
#endif
