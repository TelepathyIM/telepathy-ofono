#include "ofonocallchannel.h"

oFonoCallChannel::oFonoCallChannel(oFonoConnection *conn, QString phoneNumber, uint targetHandle, QString voiceObj, QObject *parent):
    OfonoVoiceCall(voiceObj),
    mConnection(conn),
    mPhoneNumber(phoneNumber),
    mTargetHandle(targetHandle)
{
    Tp::BaseChannelPtr baseChannel = Tp::BaseChannel::create(mConnection, TP_QT_IFACE_CHANNEL_TYPE_CALL, targetHandle, Tp::HandleTypeContact);
    Tp::BaseChannelCallTypePtr callType = Tp::BaseChannelCallType::create(baseChannel.data(),
                                                                          true,
                                                                          Tp::StreamTransportTypeUnknown,
                                                                          true,
                                                                          false, "","");
    baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(callType));

    mHoldIface = Tp::BaseChannelHoldInterface::create();
    mHoldIface->setSetHoldStateCallback(Tp::memFun(this,&oFonoCallChannel::onHoldStateChanged));

    mMuteIface = Tp::BaseCallMuteInterface::create();
    mMuteIface->setSetMuteStateCallback(Tp::memFun(this,&oFonoCallChannel::onMuteStateChanged));

    baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(mHoldIface));
    baseChannel->plugInterface(Tp::AbstractChannelInterfacePtr::dynamicCast(mMuteIface));

    mBaseChannel = baseChannel;
    mCallChannel = Tp::BaseChannelCallTypePtr::dynamicCast(mBaseChannel->interface(TP_QT_IFACE_CHANNEL_TYPE_CALL));

    mCallChannel->setHangupCallback(Tp::memFun(this,&oFonoCallChannel::onHangup));
    mCallChannel->setAcceptCallback(Tp::memFun(this,&oFonoCallChannel::onAccept));

    QTimer::singleShot(0, this, SLOT(init()));
}

void oFonoCallChannel::onHangup(uint reason, const QString &detailedReason, const QString &message, Tp::DBusError*error)
{
    hangup();
}

void oFonoCallChannel::onAccept(Tp::DBusError*)
{
    answer();
}

void oFonoCallChannel::init()
{
    bool incoming = this->state() == "incoming";
    mObjPath = mBaseChannel->objectPath();

    Tp::CallMemberMap memberFlags;
    Tp::HandleIdentifierMap identifiers;
    QVariantMap stateDetails;
    Tp::CallStateReason reason;

    identifiers[mTargetHandle] = mPhoneNumber;
    reason.actor =  0;
    reason.reason = Tp::CallStateChangeReasonProgressMade;
    reason.message = "";
    reason.DBusReason = "";
    if (incoming) {
        memberFlags[mTargetHandle] = 0;
    } else {
        memberFlags[mTargetHandle] = Tp::CallMemberFlagRinging;
    }

    mCallChannel->setCallState(Tp::CallStateInitialising, 0, reason, stateDetails);

    mCallContent = mCallChannel->addContent("audio", Tp::MediaStreamTypeAudio, Tp::MediaStreamDirectionNone);

    mCallChannel->setMembersFlags(memberFlags, identifiers, Tp::UIntList(), reason);

    mCallChannel->setCallState(Tp::CallStateInitialised, 0, reason, stateDetails);
    QObject::connect(mBaseChannel.data(), SIGNAL(closed()), this, SLOT(deleteLater()));
    QObject::connect(mConnection->callVolume(), SIGNAL(mutedChanged(bool)), SLOT(onOfonoMuteChanged(bool)));
    QObject::connect(this, SIGNAL(stateChanged(QString)), SLOT(oFonoCallStateChanged(QString)));
}

void oFonoCallChannel::onOfonoMuteChanged(bool mute)
{
    Tp::LocalMuteState state = mute ? Tp::LocalMuteStateMuted : Tp::LocalMuteStateUnmuted;
    mMuteIface->setMuteState(state);
}

void oFonoCallChannel::onHoldStateChanged(const Tp::LocalHoldState &state, const Tp::LocalHoldStateReason &reason, Tp::DBusError *error)
{
    if (state == Tp::LocalHoldStateHeld && this->state() == "active") {
        mConnection->voiceCallManager()->swapCalls();
    } else if (state == Tp::LocalHoldStateUnheld && this->state() == "held") {
        mConnection->voiceCallManager()->swapCalls();
    }
}

void oFonoCallChannel::onMuteStateChanged(const Tp::LocalMuteState &state, Tp::DBusError *error)
{
    if (state == Tp::LocalMuteStateMuted) {
        mConnection->callVolume()->setMuted(true);
    } else if (state == Tp::LocalMuteStateUnmuted) {
        mConnection->callVolume()->setMuted(false);
    }
}

oFonoCallChannel::~oFonoCallChannel()
{
    qDebug() << "call channel closed";
    // TODO - for some reason the object is not being removed
    mConnection->dbusConnection().unregisterObject(mObjPath, QDBusConnection::UnregisterTree);
}

Tp::BaseChannelPtr oFonoCallChannel::baseChannel()
{
    return mBaseChannel;
}

void oFonoCallChannel::oFonoCallStateChanged(const QString &state)
{
    Tp::CallStateReason reason;
    QVariantMap stateDetails;
    reason.actor =  0;
    reason.reason = Tp::CallStateChangeReasonProgressMade;
    reason.message = "";
    reason.DBusReason = "";
    if (state == "disconnected") {
        qDebug() << "disconnected";
        mCallChannel->setCallState(Tp::CallStateEnded, 0, reason, stateDetails);
        Q_EMIT mBaseChannel->closed();
    } else if (state == "active") {
        qDebug() << "active";
        mCallChannel->setCallState(Tp::CallStateActive, 0, reason, stateDetails);
        mHoldIface->setHoldState(Tp::LocalHoldStateUnheld, Tp::LocalHoldStateReasonNone);
    } else if (state == "held") {
        mHoldIface->setHoldState(Tp::LocalHoldStateHeld, Tp::LocalHoldStateReasonNone);
        qDebug() << "held";
    } else if (state == "dialing") {
        qDebug() << "dialing";
    } else if (state == "alerting") {
        qDebug() << "alerting";
    } else if (state == "incoming") {
        qDebug() << "incoming";
    } else if (state == "waiting") {
        qDebug() << "waiting";
    }
}
