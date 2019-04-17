// Copyright (c) 2014-2017 The Dash Core developers
// Copyright (c) 2018 FXTC developers
// Copyright (c) 2018-2019 SUQA developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/masternodelist.h>
#include <qt/forms/ui_masternodelist.h>

#include <activemasternode.h>
#include <interfaces/wallet.h>
#include <qt/clientmodel.h>
#include <qt/guiutil.h>
#include <init.h>
#include <key_io.h>
#include <core_io.h>
#include <masternode-sync.h>
#include <masternodeconfig.h>
#include <masternodeman.h>
#include <sync.h>
#include <wallet/wallet.h>
#include <qt/walletmodel.h>

#include <QDialog>
#include <QInputDialog>
#include <QTimer>
#include <QMessageBox>

int GetOffsetFromUtc()
{
#if QT_VERSION < 0x050200
    const QDateTime dateTime1 = QDateTime::currentDateTime();
    const QDateTime dateTime2 = QDateTime(dateTime1.date(), dateTime1.time(), Qt::UTC);
    return dateTime1.secsTo(dateTime2);
#else
    return QDateTime::currentDateTime().offsetFromUtc();
#endif
}

MasternodeList::MasternodeList(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MasternodeList),
    clientModel(0),
    walletModel(0)
{
    ui->setupUi(this);

    ui->startButton->setEnabled(false);

    int columnAliasWidth = 100;
    int columnAddressWidth = 200;
    int columnProtocolWidth = 60;
    int columnStatusWidth = 80;
    int columnActiveWidth = 130;
    int columnLastSeenWidth = 130;

    ui->tableWidgetMyMasternodes->setColumnWidth(0, columnAliasWidth);
    ui->tableWidgetMyMasternodes->setColumnWidth(1, columnAddressWidth);
    ui->tableWidgetMyMasternodes->setColumnWidth(2, columnProtocolWidth);
    ui->tableWidgetMyMasternodes->setColumnWidth(3, columnStatusWidth);
    ui->tableWidgetMyMasternodes->setColumnWidth(4, columnActiveWidth);
    ui->tableWidgetMyMasternodes->setColumnWidth(5, columnLastSeenWidth);

    ui->tableWidgetMasternodes->setColumnWidth(0, columnAddressWidth);
    ui->tableWidgetMasternodes->setColumnWidth(1, columnProtocolWidth);
    ui->tableWidgetMasternodes->setColumnWidth(2, columnStatusWidth);
    ui->tableWidgetMasternodes->setColumnWidth(3, columnActiveWidth);
    ui->tableWidgetMasternodes->setColumnWidth(4, columnLastSeenWidth);

    ui->tableWidgetMyMasternodes->setContextMenuPolicy(Qt::CustomContextMenu);

    QAction *startAliasAction = new QAction(tr("Start alias"), this);
    contextMenu = new QMenu();
    contextMenu->addAction(startAliasAction);
    connect(ui->tableWidgetMyMasternodes, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&)));
    connect(startAliasAction, SIGNAL(triggered()), this, SLOT(on_startButton_clicked()));

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateNodeList()));
    connect(timer, SIGNAL(timeout()), this, SLOT(updateMyNodeList()));
    timer->start(1000);

    fFilterUpdated = false;
    nTimeFilterUpdated = GetTime();
    updateNodeList();
}

MasternodeList::~MasternodeList()
{
    delete ui;
}

void MasternodeList::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if(model) {
        // try to update list when masternode count changes
        connect(clientModel, SIGNAL(strMasternodesChanged(QString)), this, SLOT(updateNodeList()));
    }
}

void MasternodeList::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
}

void MasternodeList::showContextMenu(const QPoint &point)
{
    QTableWidgetItem *item = ui->tableWidgetMyMasternodes->itemAt(point);
    if(item) contextMenu->exec(QCursor::pos());
}

void MasternodeList::StartAlias(std::string strAlias)
{
    std::string strStatusHtml;
    strStatusHtml += "<center>Alias: " + strAlias;

    for (CMasternodeConfig::CMasternodeEntry mne : masternodeConfig.getEntries()) {
        if(mne.getAlias() == strAlias) {
            std::string strError;
            CMasternodeBroadcast mnb;

            bool fSuccess = CMasternodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), mne.getTxHashBurnFund(), mne.getOutputIndexBurnFund(), strError, mnb);

            if(fSuccess) {
                strStatusHtml += "<br>Successfully started infinitynode.";
                mnodeman.UpdateMasternodeList(mnb, *g_connman);
                mnb.Relay(*g_connman);
                mnodeman.NotifyMasternodeUpdates(*g_connman);
            } else {
                strStatusHtml += "<br>Failed to start infinitynode.<br>Error: " + strError;
            }
            break;
        }
    }
    strStatusHtml += "</center>";

    QMessageBox msg;
    msg.setText(QString::fromStdString(strStatusHtml));
    msg.exec();

    updateMyNodeList(true);
}

void MasternodeList::StartAll(std::string strCommand)
{
    int nCountSuccessful = 0;
    int nCountFailed = 0;
    std::string strFailedHtml;

    for (CMasternodeConfig::CMasternodeEntry mne : masternodeConfig.getEntries()) {
        std::string strError;
        CMasternodeBroadcast mnb;

        int32_t nOutputIndex = 0;
        if(!ParseInt32(mne.getOutputIndex(), &nOutputIndex)) {
            continue;
        }

        COutPoint outpoint = COutPoint(uint256S(mne.getTxHash()), nOutputIndex);

        if(strCommand == "start-missing" && mnodeman.Has(outpoint)) continue;

        bool fSuccess = CMasternodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), mne.getTxHashBurnFund(), mne.getOutputIndexBurnFund(), strError, mnb);

        if(fSuccess) {
            nCountSuccessful++;
            mnodeman.UpdateMasternodeList(mnb, *g_connman);
            mnb.Relay(*g_connman);
            mnodeman.NotifyMasternodeUpdates(*g_connman);
        } else {
            nCountFailed++;
            strFailedHtml += "\nFailed to start " + mne.getAlias() + ". Error: " + strError;
        }
    }
    std::vector<std::shared_ptr<CWallet>> wallets = GetWallets();
    CWallet * const pwallet = (wallets.size() > 0) ? wallets[0].get() : nullptr;
    pwallet->Lock();

    std::string returnObj;
    returnObj = strprintf("Successfully started %d infinitynodes, failed to start %d, total %d", nCountSuccessful, nCountFailed, nCountFailed + nCountSuccessful);
    if (nCountFailed > 0) {
        returnObj += strFailedHtml;
    }

    QMessageBox msg;
    msg.setText(QString::fromStdString(returnObj));
    msg.exec();

    updateMyNodeList(true);
}

void MasternodeList::updateMyMasternodeInfo(QString strAlias, QString strAddr, const COutPoint& outpoint)
{
    bool fOldRowFound = false;
    int nNewRow = 0;

    for(int i = 0; i < ui->tableWidgetMyMasternodes->rowCount(); i++) {
        if(ui->tableWidgetMyMasternodes->item(i, 0)->text() == strAlias) {
            fOldRowFound = true;
            nNewRow = i;
            break;
        }
    }

    if(nNewRow == 0 && !fOldRowFound) {
        nNewRow = ui->tableWidgetMyMasternodes->rowCount();
        ui->tableWidgetMyMasternodes->insertRow(nNewRow);
    }

    masternode_info_t infoMn;
    bool fFound = mnodeman.GetMasternodeInfo(outpoint, infoMn);

    QTableWidgetItem *aliasItem = new QTableWidgetItem(strAlias);
    QTableWidgetItem *addrItem = new QTableWidgetItem(fFound ? QString::fromStdString(infoMn.addr.ToString()) : strAddr);
    QTableWidgetItem *protocolItem = new QTableWidgetItem(QString::number(fFound ? infoMn.nProtocolVersion : -1));
    QTableWidgetItem *statusItem = new QTableWidgetItem(QString::fromStdString(fFound ? CMasternode::StateToString(infoMn.nActiveState) : "MISSING"));
    QTableWidgetItem *activeSecondsItem = new QTableWidgetItem(QString::fromStdString(DurationToDHMS(fFound ? (infoMn.nTimeLastPing - infoMn.sigTime) : 0)));
    QTableWidgetItem *lastSeenItem = new QTableWidgetItem(QString::fromStdString(FormatISO8601DateTime(fFound ? infoMn.nTimeLastPing + GetOffsetFromUtc() : 0)));
    QTableWidgetItem *pubkeyItem = new QTableWidgetItem(QString::fromStdString(fFound ? EncodeDestination(infoMn.pubKeyCollateralAddress.GetID()) : ""));

    ui->tableWidgetMyMasternodes->setItem(nNewRow, 0, aliasItem);
    ui->tableWidgetMyMasternodes->setItem(nNewRow, 1, addrItem);
    ui->tableWidgetMyMasternodes->setItem(nNewRow, 2, protocolItem);
    ui->tableWidgetMyMasternodes->setItem(nNewRow, 3, statusItem);
    ui->tableWidgetMyMasternodes->setItem(nNewRow, 4, activeSecondsItem);
    ui->tableWidgetMyMasternodes->setItem(nNewRow, 5, lastSeenItem);
    ui->tableWidgetMyMasternodes->setItem(nNewRow, 6, pubkeyItem);
}

void MasternodeList::updateMyNodeList(bool fForce)
{
    TRY_LOCK(cs_mymnlist, fLockAcquired);
    if(!fLockAcquired) {
        return;
    }
    static int64_t nTimeMyListUpdated = 0;

    // automatically update my masternode list only once in MY_MASTERNODELIST_UPDATE_SECONDS seconds,
    // this update still can be triggered manually at any time via button click
    int64_t nSecondsTillUpdate = nTimeMyListUpdated + MY_MASTERNODELIST_UPDATE_SECONDS - GetTime();
    ui->secondsLabel->setText(QString::number(nSecondsTillUpdate));

    if(nSecondsTillUpdate > 0 && !fForce) return;
    nTimeMyListUpdated = GetTime();

    ui->tableWidgetMasternodes->setSortingEnabled(false);
    for (CMasternodeConfig::CMasternodeEntry mne : masternodeConfig.getEntries()) {
        int32_t nOutputIndex = 0;
        if(!ParseInt32(mne.getOutputIndex(), &nOutputIndex)) {
            continue;
        }

        updateMyMasternodeInfo(QString::fromStdString(mne.getAlias()), QString::fromStdString(mne.getIp()), COutPoint(uint256S(mne.getTxHash()), nOutputIndex));
    }
    ui->tableWidgetMasternodes->setSortingEnabled(true);

    // reset "timer"
    ui->secondsLabel->setText("0");
}

void MasternodeList::updateNodeList()
{
    TRY_LOCK(cs_mnlist, fLockAcquired);
    if(!fLockAcquired) {
        return;
    }

    static int64_t nTimeListUpdated = GetTime();

    // to prevent high cpu usage update only once in MASTERNODELIST_UPDATE_SECONDS seconds
    // or MASTERNODELIST_FILTER_COOLDOWN_SECONDS seconds after filter was last changed
    int64_t nSecondsToWait = fFilterUpdated
                            ? nTimeFilterUpdated - GetTime() + MASTERNODELIST_FILTER_COOLDOWN_SECONDS
                            : nTimeListUpdated - GetTime() + MASTERNODELIST_UPDATE_SECONDS;

    if(fFilterUpdated) ui->countLabel->setText(QString::fromStdString(strprintf("Please wait... %d", nSecondsToWait)));
    if(nSecondsToWait > 0) return;

    nTimeListUpdated = GetTime();
    fFilterUpdated = false;

    QString strToFilter;
    ui->countLabel->setText("Updating...");
    ui->tableWidgetMasternodes->setSortingEnabled(false);
    ui->tableWidgetMasternodes->clearContents();
    ui->tableWidgetMasternodes->setRowCount(0);
    std::map<COutPoint, CMasternode> mapMasternodes = mnodeman.GetFullMasternodeMap();
    int offsetFromUtc = GetOffsetFromUtc();

    for(auto& mnpair : mapMasternodes)
    {
        CMasternode mn = mnpair.second;
        // populate list
        // Address, Protocol, Status, Active Seconds, Last Seen, Pub Key
        QTableWidgetItem *addressItem = new QTableWidgetItem(QString::fromStdString(mn.addr.ToString()));
        QTableWidgetItem *protocolItem = new QTableWidgetItem(QString::number(mn.nProtocolVersion));
        QTableWidgetItem *statusItem = new QTableWidgetItem(QString::fromStdString(mn.GetStatus()));
        QTableWidgetItem *activeSecondsItem = new QTableWidgetItem(QString::fromStdString(DurationToDHMS(mn.lastPing.sigTime - mn.sigTime)));
        QTableWidgetItem *lastSeenItem = new QTableWidgetItem(QString::fromStdString(FormatISO8601DateTime(mn.lastPing.sigTime + offsetFromUtc)));
        QTableWidgetItem *pubkeyItem = new QTableWidgetItem(QString::fromStdString(EncodeDestination(mn.pubKeyCollateralAddress.GetID())));

        if (strCurrentFilter != "")
        {
            strToFilter =   addressItem->text() + " " +
                            protocolItem->text() + " " +
                            statusItem->text() + " " +
                            activeSecondsItem->text() + " " +
                            lastSeenItem->text() + " " +
                            pubkeyItem->text();
            if (!strToFilter.contains(strCurrentFilter)) continue;
        }

        ui->tableWidgetMasternodes->insertRow(0);
        ui->tableWidgetMasternodes->setItem(0, 0, addressItem);
        ui->tableWidgetMasternodes->setItem(0, 1, protocolItem);
        ui->tableWidgetMasternodes->setItem(0, 2, statusItem);
        ui->tableWidgetMasternodes->setItem(0, 3, activeSecondsItem);
        ui->tableWidgetMasternodes->setItem(0, 4, lastSeenItem);
        ui->tableWidgetMasternodes->setItem(0, 5, pubkeyItem);
    }

    ui->countLabel->setText(QString::number(ui->tableWidgetMasternodes->rowCount()));
    ui->tableWidgetMasternodes->setSortingEnabled(true);
}

void MasternodeList::on_filterLineEdit_textChanged(const QString &strFilterIn)
{
    strCurrentFilter = strFilterIn;
    nTimeFilterUpdated = GetTime();
    fFilterUpdated = true;
    ui->countLabel->setText(QString::fromStdString(strprintf("Please wait... %d", MASTERNODELIST_FILTER_COOLDOWN_SECONDS)));
}

void MasternodeList::on_startButton_clicked()
{
    std::string strAlias;
    {
        LOCK(cs_mymnlist);
        // Find selected node alias
        QItemSelectionModel* selectionModel = ui->tableWidgetMyMasternodes->selectionModel();
        QModelIndexList selected = selectionModel->selectedRows();

        if(selected.count() == 0) return;

        QModelIndex index = selected.at(0);
        int nSelectedRow = index.row();
        strAlias = ui->tableWidgetMyMasternodes->item(nSelectedRow, 0)->text().toStdString();
    }

    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm infinitynode start"),
        tr("Are you sure you want to start infinitynode %1?").arg(QString::fromStdString(strAlias)),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if(retval != QMessageBox::Yes) return;

    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();

    if(encStatus == walletModel->Locked || encStatus == walletModel->UnlockedForMixingOnly) {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());

        if(!ctx.isValid()) return; // Unlock wallet was cancelled

        StartAlias(strAlias);
        return;
    }

    StartAlias(strAlias);
}

void MasternodeList::on_startAllButton_clicked()
{
    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm all Infinitynodes start"),
        tr("Are you sure you want to start ALL InfinityNodes?"),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if(retval != QMessageBox::Yes) return;

    WalletModel::EncryptionStatus encStatus = walletModel->getEncryptionStatus();

    if(encStatus == walletModel->Locked || encStatus == walletModel->UnlockedForMixingOnly) {
        WalletModel::UnlockContext ctx(walletModel->requestUnlock());

        if(!ctx.isValid()) return; // Unlock wallet was cancelled

        StartAll();
        return;
    }

    StartAll();
}

void MasternodeList::on_startAutoSINButton_clicked()
{
    std::vector<std::shared_ptr<CWallet>> wallets = GetWallets();
    CWallet * const pwallet = (wallets.size() > 0) ? wallets[0].get() : nullptr;

    bool ok;
    setStyleSheet( "QDialog{ background-color: #0d1827; }");
    QString vpsip = QInputDialog::getText(this, tr("SINnode"), tr("Enter VPS address:"), QLineEdit::Normal, "", &ok);

    if (!ok)
       return;
    // quick input parsing
    int vpsiplen = strlen(vpsip.toUtf8().constData());
    if (vpsiplen < 7 || vpsiplen > 16)
       return;
    // char type parsing
    for (int i=0; i<vpsiplen; i++)
       if ((vpsip[i] < 46 || vpsip[i] > 57) || vpsip[i] == 47)
          return;

    // generate masternode key
    CKey secret;
    std::vector<infinitynode_conf_t> listNode;
    std::set<uint256> trackCollateralTx;

    secret.MakeNewKey(false);

    bool foundCollat = false;
    CTxDestination collateralAddress = CTxDestination();

    // find suitable burntx
    bool foundBurn = false;
    int counter = 0;

    for (map<uint256, CWalletTx>::const_iterator it = pwallet->mapWallet.begin(); it != pwallet->mapWallet.end(); ++it) {
      const uint256* txid = &(*it).first;
      const CWalletTx* pcoin = &(*it).second;
      for (unsigned int i = 0; i < pcoin->tx->vout.size(); i++) {
          CTxDestination address;
          bool fValidAddress = ExtractDestination(pcoin->tx->vout[i].scriptPubKey, address);
          CTxDestination BurnAddress = DecodeDestination(Params().GetConsensus().cBurnAddress);
            if (
                (address == BurnAddress) &&
                (
                    ((Params().GetConsensus().nMasternodeBurnSINNODE_1 - 1) * COIN < pcoin->tx->vout[i].nValue && pcoin->tx->vout[i].nValue <= Params().GetConsensus().nMasternodeBurnSINNODE_1 * COIN) ||
                    ((Params().GetConsensus().nMasternodeBurnSINNODE_5 - 1) < pcoin->tx->vout[i].nValue * COIN && pcoin->tx->vout[i].nValue <= Params().GetConsensus().nMasternodeBurnSINNODE_5 * COIN) ||
                    ((Params().GetConsensus().nMasternodeBurnSINNODE_10 - 1) * COIN < pcoin->tx->vout[i].nValue && pcoin->tx->vout[i].nValue <= Params().GetConsensus().nMasternodeBurnSINNODE_10 * COIN)
                )
            ) {
                    //add dummy
                    listNode.push_back(infinitynode_conf_t());
                    foundBurn = true;

                    //add to list
                    listNode[counter].burnFundHash = txid->ToString();
                    listNode[counter].burnFundIndex = i;

                    const CTxIn& txin = pcoin->tx->vin[0]; //Burn Input is only one address. So we can take the first without problem
                    string strAsm = ScriptToAsmStr(txin.scriptSig, true);
                    string s;
                    stringstream ss(strAsm);
                    int i=0;
                    while (getline(ss, s,' ')) {
                        if (i==1) {
                            std::vector<unsigned char> data(ParseHex(s));
                            CPubKey pubKey(data.begin(), data.end());
                            if (!pubKey.IsFullyValid()) {
                                LogPrintf("MasternodeList::AutoSIN -- Can't not find Input Pubkey key.\n");
                                return;
                            } else {
                                //LogPrintf("CMasternode::BurnFundStatus -- Pubkey is correct\n");
                                OutputType output_type = OutputType::LEGACY;
                                collateralAddress = GetDestinationForKey(pubKey, output_type);
                                listNode[counter].collateralAddress = collateralAddress;
                                secret.MakeNewKey(false);
                                listNode[counter].infinitynodePrivateKey = EncodeSecret(secret);
                                listNode[counter].IPaddress = vpsip.toUtf8().constData();
                                listNode[counter].port = Params().GetDefaultPort();
                            }
                        }
                        i++;
                    }
                    counter++; //new item in list if found
            }
        }
    }

    // find suitable collateral outputs
    std::vector<COutput> vPossibleCoins;
    LOCK2(cs_main, pwallet->cs_wallet);
    pwallet->AvailableCoins(vPossibleCoins, true, NULL, false, ONLY_MASTERNODE_COLLATERAL);

    for (COutput& out : vPossibleCoins) {
      CTxDestination address;
        const CScript& scriptPubKey = out.tx->tx->vout[out.i].scriptPubKey;
        bool fValidAddress = ExtractDestination(scriptPubKey, address);
        for (unsigned int i = 0; i < listNode.size(); i++) {
            if (address == listNode[i].collateralAddress && trackCollateralTx.count(out.tx->GetHash()) != 1) {
                listNode[i].collateralHash = out.tx->GetHash().ToString();
                listNode[i].collateralIndex = out.i;
                trackCollateralTx.insert(out.tx->GetHash());
                foundCollat = true;
            }
        }
    }

    if (!foundBurn) {
        LogPrintf("MasternodeList::AutoSIN -- burnTx not found\n");
    }

    if (!foundCollat) {
        LogPrintf("MasternodeList::AutoSIN -- collateral not found\n");
    }

    if (foundCollat && foundBurn) {
       boost::filesystem::path pathMasternodeConfigFile = GetMasternodeConfigFile();
       boost::filesystem::ifstream streamConfig(pathMasternodeConfigFile);
       FILE* configFile = fopen(pathMasternodeConfigFile.string().c_str(), "w");
       std::string strHeader = "# infinitynode config file\n"
                               "# Format: alias IP:port infinitynodeprivkey collateral_output_txid collateral_output_index burnfund_output_txid burnfund_output_index\n"
                               "# infinitynode1 127.0.0.1:20980 7RVuQhi45vfazyVtskTRLBgNuSrYGecS5zj2xERaooFVnWKKjhS b7ed8c1396cf57ac78d756186b6022d3023fd2f1c338b7fbae42d342fdd7070a 0 563d9434e816b3e8ffc5347c6b8db07509de6068f6759f21a16be5d92b7e3111 1\n";
       fwrite(strHeader.c_str(), std::strlen(strHeader.c_str()), 1, configFile);
       for (unsigned int i = 0; i < listNode.size(); i++) {
           char inconfigline[300];
           memset(inconfigline,'\0',300);
           sprintf(inconfigline,"infinitynode%d %s:%d %s %s %d %s %d %s\n",i, listNode[i].IPaddress.c_str(), listNode[i].port, listNode[i].infinitynodePrivateKey.c_str(), listNode[i].collateralHash.c_str(), listNode[i].collateralIndex, listNode[i].burnFundHash.c_str(), listNode[i].burnFundIndex, EncodeDestination(listNode[i].collateralAddress).c_str());
           fwrite(inconfigline, strlen(inconfigline), 1, configFile);
       }
       fclose(configFile);
    }
}

void MasternodeList::on_tableWidgetMyMasternodes_itemSelectionChanged()
{
    if(ui->tableWidgetMyMasternodes->selectedItems().count() > 0) {
        ui->startButton->setEnabled(true);
    }
}

void MasternodeList::on_UpdateButton_clicked()
{
    updateMyNodeList(true);
}
