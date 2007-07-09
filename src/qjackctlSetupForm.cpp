// qjackctlSetupForm.cpp
//
/****************************************************************************
   Copyright (C) 2003-2007, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*****************************************************************************/

#include "qjackctlAbout.h"
#include "qjackctlSetupForm.h"

#include "qjackctlSetup.h"

#include <QValidator>
#include <QMessageBox>
#include <QFileDialog>
#include <QFontDialog>
#include <QTextStream>
#include <QLineEdit>
#include <QPixmap>
#include <QMenu>

#include <QButtonGroup>


#ifdef CONFIG_COREAUDIO
#include <iostream>
#include <cstring>
#include <map>
#include <CoreAudio/CoreAudio.h>
#include <CoreFoundation/CFString.h>
#endif

#ifdef CONFIG_ALSA_SEQ
#include <alsa/asoundlib.h>
#endif


//----------------------------------------------------------------------------
// qjackctlSetupForm -- UI wrapper form.

// Constructor.
qjackctlSetupForm::qjackctlSetupForm (
	QWidget *pParent, Qt::WindowFlags wflags )
	: QDialog(pParent, wflags)
{
	// Setup UI struct...
	m_ui.setupUi(this);

	// No settings descriptor initially (the caller will set it).
	m_pSetup = NULL;

	// Setup time-display radio-button group.
	m_pTimeDisplayButtonGroup = new QButtonGroup(this);
	m_pTimeDisplayButtonGroup->addButton(m_ui.TransportTimeRadioButton, 0);
	m_pTimeDisplayButtonGroup->addButton(m_ui.TransportBBTRadioButton,  1);
	m_pTimeDisplayButtonGroup->addButton(m_ui.ElapsedResetRadioButton,  2);
	m_pTimeDisplayButtonGroup->addButton(m_ui.ElapsedXrunRadioButton,   3);
	m_pTimeDisplayButtonGroup->setExclusive(true);

	// Initialize dirty control state.
	m_iDirtySetup = 0;
	m_iDirtySettings = 0;
	m_iDirtyOptions = 0;

	// Set dialog validators...
	m_ui.PresetComboBox->setValidator(
		new QRegExpValidator(QRegExp("[\\w-]+"), m_ui.PresetComboBox));
	m_ui.FramesComboBox->setValidator(
		new QIntValidator(m_ui.FramesComboBox));
	m_ui.SampleRateComboBox->setValidator(
		new QIntValidator(m_ui.SampleRateComboBox));
	m_ui.WaitComboBox->setValidator(
		new QIntValidator(m_ui.WaitComboBox));
	m_ui.WordLengthComboBox->setValidator(
		new QIntValidator(m_ui.WordLengthComboBox));
	m_ui.TimeoutComboBox->setValidator(
		new QIntValidator(m_ui.TimeoutComboBox));
	m_ui.TimeRefreshComboBox->setValidator(
		new QIntValidator(m_ui.TimeRefreshComboBox));
	m_ui.PortMaxComboBox->setValidator(
		new QIntValidator(m_ui.PortMaxComboBox));
	m_ui.MessagesLimitLinesComboBox->setValidator(
		new QIntValidator(m_ui.MessagesLimitLinesComboBox));

	// UI connections...

	QObject::connect(m_ui.PresetComboBox,
		SIGNAL(textChanged(const QString&)),
		SLOT(changeCurrentPreset(const QString&)));
	QObject::connect(m_ui.PresetSavePushButton,
		SIGNAL(clicked()),
		SLOT(saveCurrentPreset()));
	QObject::connect(m_ui.PresetDeletePushButton,
		SIGNAL(clicked()),
		SLOT(deleteCurrentPreset()));

	QObject::connect(m_ui.DriverComboBox,
		SIGNAL(highlighted(const QString&)),
		SLOT(changeDriver(const QString&)));
	QObject::connect(m_ui.InterfaceToolButton,
		SIGNAL(clicked()),
		SLOT(selectInterface()));
	QObject::connect(m_ui.InDeviceToolButton,
		SIGNAL(clicked()),
		SLOT(selectInDevice()));
	QObject::connect(m_ui.OutDeviceToolButton,
		SIGNAL(clicked()),
		SLOT(selectOutDevice()));
	QObject::connect(m_ui.AudioComboBox,
		SIGNAL(highlighted(int)),
		SLOT(changeAudio(int)));
	QObject::connect(m_ui.ServerComboBox,
		SIGNAL(textChanged(const QString&)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.DriverComboBox,
		SIGNAL(activated(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.RealtimeCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.NoMemLockCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.SoftModeCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.MonitorCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.ShortsCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.HWMonCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.HWMeterCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.IgnoreHWCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.UnlockMemCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.VerboseCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.PrioritySpinBox,
		SIGNAL(valueChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.FramesComboBox,
		SIGNAL(textChanged(const QString&)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.SampleRateComboBox,
		SIGNAL(textChanged(const QString&)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.PeriodsSpinBox,
		SIGNAL(valueChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.WordLengthComboBox,
		SIGNAL(textChanged(const QString&)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.WaitComboBox,
		SIGNAL(textChanged(const QString&)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.ChanSpinBox,
		SIGNAL(valueChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.InterfaceComboBox,
		SIGNAL(textChanged(const QString&)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.AudioComboBox,
		SIGNAL(activated(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.DitherComboBox,
		SIGNAL(activated(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.TimeoutComboBox,
		SIGNAL(textChanged(const QString&)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.InDeviceComboBox,
		SIGNAL(textChanged(const QString&)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.OutDeviceComboBox,
		SIGNAL(textChanged(const QString&)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.InChannelsSpinBox,
		SIGNAL(valueChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.OutChannelsSpinBox,
		SIGNAL(valueChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.InLatencySpinBox,
		SIGNAL(valueChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.OutLatencySpinBox,
		SIGNAL(valueChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.MidiDriverComboBox,
		SIGNAL(activated(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.StartDelaySpinBox,
		SIGNAL(valueChanged(int)),
		SLOT(settingsChanged()));
	QObject::connect(m_ui.PortMaxComboBox,
		SIGNAL(textChanged(const QString&)),
		SLOT(settingsChanged()));

	QObject::connect(m_ui.StartupScriptCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.StartupScriptShellComboBox,
		SIGNAL(textChanged(const QString&)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.PostStartupScriptCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.PostStartupScriptShellComboBox,
		SIGNAL(textChanged(const QString&)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.ShutdownScriptCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.ShutdownScriptShellComboBox,
		SIGNAL(textChanged(const QString&)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.PostShutdownScriptCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.PostShutdownScriptShellComboBox,
		SIGNAL(textChanged(const QString&)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.StartupScriptBrowseToolButton,
		SIGNAL(clicked()),
		SLOT(browseStartupScript()));
	QObject::connect(m_ui.PostStartupScriptBrowseToolButton,
		SIGNAL(clicked()),
		SLOT(browsePostStartupScript()));
	QObject::connect(m_ui.ShutdownScriptBrowseToolButton,
		SIGNAL(clicked()),
		SLOT(browseShutdownScript()));
	QObject::connect(m_ui.PostShutdownScriptBrowseToolButton,
		SIGNAL(clicked()),
		SLOT(browsePostShutdownScript()));
	QObject::connect(m_ui.StartupScriptSymbolToolButton,
		SIGNAL(clicked()),
		SLOT(symbolStartupScript()));
	QObject::connect(m_ui.PostStartupScriptSymbolToolButton,
		SIGNAL(clicked()),
		SLOT(symbolPostStartupScript()));
	QObject::connect(m_ui.ShutdownScriptSymbolToolButton,
		SIGNAL(clicked()),
		SLOT(symbolShutdownScript()));
	QObject::connect(m_ui.PostShutdownScriptSymbolToolButton,
		SIGNAL(clicked()),
		SLOT(symbolPostShutdownScript()));

	QObject::connect(m_ui.StdoutCaptureCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.XrunRegexComboBox,
		SIGNAL(textChanged(const QString&)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.XrunIgnoreFirstCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.ActivePatchbayCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.ActivePatchbayPathComboBox,
		SIGNAL(textChanged(const QString&)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.ActivePatchbayPathToolButton,
		SIGNAL(clicked()),
		SLOT(browseActivePatchbayPath()));

	QObject::connect(m_ui.AutoRefreshCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.TimeRefreshComboBox,
		SIGNAL(textChanged(const QString&)),
		SLOT(optionsChanged()));

	QObject::connect(m_ui.TransportTimeRadioButton,
		SIGNAL(toggled(bool)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.TransportBBTRadioButton,
		SIGNAL(toggled(bool)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.ElapsedResetRadioButton,
		SIGNAL(toggled(bool)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.ElapsedXrunRadioButton,
		SIGNAL(toggled(bool)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.TimeFormatComboBox,
		SIGNAL(activated(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.DisplayEffectCheckBox,
		SIGNAL(toggled(bool)),
		SLOT(toggleDisplayEffect(bool)));
	QObject::connect(m_ui.DisplayFont1PushButton,
		SIGNAL(clicked()),
		SLOT(chooseDisplayFont1()));
	QObject::connect(m_ui.DisplayFont2PushButton,
		SIGNAL(clicked()),
		SLOT(chooseDisplayFont2()));
	QObject::connect(m_ui.MessagesFontPushButton,
		SIGNAL(clicked()),
		SLOT(chooseMessagesFont()));
	QObject::connect(m_ui.ConnectionsFontPushButton,
		SIGNAL(clicked()),
		SLOT(chooseConnectionsFont()));
	QObject::connect(m_ui.MessagesLimitCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.MessagesLimitLinesComboBox,
		SIGNAL(textChanged(const QString&)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.ConnectionsIconSizeComboBox,
		SIGNAL(activated(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.BezierLinesCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.AliasesEnabledCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.AliasesEditingCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));

	QObject::connect(m_ui.StartJackCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.QueryCloseCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.KeepOnTopCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.SystemTrayCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.DelayedSetupCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.ServerConfigCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.ServerConfigNameComboBox,
		SIGNAL(textChanged(const QString&)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.ServerConfigTempCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.QueryShutdownCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.LeftButtonsCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.RightButtonsCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.TransportButtonsCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));
	QObject::connect(m_ui.TextLabelsCheckBox,
		SIGNAL(stateChanged(int)),
		SLOT(optionsChanged()));

	QObject::connect(m_ui.OkPushButton,
		SIGNAL(clicked()),
		SLOT(accept()));
	QObject::connect(m_ui.CancelPushButton,
		SIGNAL(clicked()),
		SLOT(reject()));

	// Try to restore old window positioning.
	adjustSize();
}


// Destructor.
qjackctlSetupForm::~qjackctlSetupForm (void)
{
	delete m_pTimeDisplayButtonGroup;
}


// A combo-box text item setter helper.
void qjackctlSetupForm::setComboBoxCurrentText (
	QComboBox *pComboBox, const QString& sText ) const
{
	if (pComboBox->isEditable()) {
		pComboBox->setEditText(sText);
	} else {
		int iIndex = pComboBox->findText(sText);
		if (iIndex < 0) {
			iIndex = 0;
			if (!sText.isEmpty())
				pComboBox->insertItem(0, sText);
		}
		pComboBox->setCurrentIndex(iIndex);
	}
}


// Populate (setup) dialog controls from settings descriptors.
void qjackctlSetupForm::setup ( qjackctlSetup *pSetup )
{
	// Set reference descriptor.
	m_pSetup = pSetup;

	// Avoid dirty this all up.
	m_iDirtySetup++;

	// Load combo box history...
	m_pSetup->loadComboBoxHistory(m_ui.ServerComboBox);
	m_pSetup->loadComboBoxHistory(m_ui.InterfaceComboBox);
	m_pSetup->loadComboBoxHistory(m_ui.InDeviceComboBox);
	m_pSetup->loadComboBoxHistory(m_ui.OutDeviceComboBox);
	m_pSetup->loadComboBoxHistory(m_ui.StartupScriptShellComboBox);
	m_pSetup->loadComboBoxHistory(m_ui.PostStartupScriptShellComboBox);
	m_pSetup->loadComboBoxHistory(m_ui.ShutdownScriptShellComboBox);
	m_pSetup->loadComboBoxHistory(m_ui.PostShutdownScriptShellComboBox);
	m_pSetup->loadComboBoxHistory(m_ui.XrunRegexComboBox);
	m_pSetup->loadComboBoxHistory(m_ui.ActivePatchbayPathComboBox);
	m_pSetup->loadComboBoxHistory(m_ui.ServerConfigNameComboBox);

	// Load Options...
	m_ui.StartupScriptCheckBox->setChecked(m_pSetup->bStartupScript);
	setComboBoxCurrentText(m_ui.StartupScriptShellComboBox,
		m_pSetup->sStartupScriptShell);
	m_ui.PostStartupScriptCheckBox->setChecked(m_pSetup->bPostStartupScript);
	setComboBoxCurrentText(m_ui.PostStartupScriptShellComboBox,
		m_pSetup->sPostStartupScriptShell);
	m_ui.ShutdownScriptCheckBox->setChecked(m_pSetup->bShutdownScript);
	setComboBoxCurrentText(m_ui.ShutdownScriptShellComboBox,
		m_pSetup->sShutdownScriptShell);
	m_ui.PostShutdownScriptCheckBox->setChecked(m_pSetup->bPostShutdownScript);
	setComboBoxCurrentText(m_ui.PostShutdownScriptShellComboBox,
		m_pSetup->sPostShutdownScriptShell);
	m_ui.StdoutCaptureCheckBox->setChecked(m_pSetup->bStdoutCapture);
	setComboBoxCurrentText(m_ui.XrunRegexComboBox,
		m_pSetup->sXrunRegex);
	m_ui.XrunIgnoreFirstCheckBox->setChecked(m_pSetup->bXrunIgnoreFirst);
	m_ui.ActivePatchbayCheckBox->setChecked(m_pSetup->bActivePatchbay);
	setComboBoxCurrentText(m_ui.ActivePatchbayPathComboBox,
		m_pSetup->sActivePatchbayPath);
	m_ui.AutoRefreshCheckBox->setChecked(m_pSetup->bAutoRefresh);
	setComboBoxCurrentText(m_ui.TimeRefreshComboBox,
		QString::number(m_pSetup->iTimeRefresh));
	m_ui.BezierLinesCheckBox->setChecked(m_pSetup->bBezierLines);

	// Load some other defaults...
	QRadioButton *pRadioButton
		= static_cast<QRadioButton *> (
			m_pTimeDisplayButtonGroup->button(m_pSetup->iTimeDisplay));
	if (pRadioButton)
		pRadioButton->setChecked(true);

	m_ui.TimeFormatComboBox->setCurrentIndex(m_pSetup->iTimeFormat);

	// Load font chooser samples...
	const QString sSansSerif = "Sans Serif";
	QFont font;
	QPalette pal;
	
	if (m_pSetup->sDisplayFont1.isEmpty()
		|| !font.fromString(m_pSetup->sDisplayFont1))
		font = QFont(sSansSerif, 12, QFont::Bold);
	m_ui.DisplayFont1TextLabel->setFont(font);
	m_ui.DisplayFont1TextLabel->setText(
		font.family() + ' ' + QString::number(font.pointSize()));

	if (m_pSetup->sDisplayFont2.isEmpty()
		|| !font.fromString(m_pSetup->sDisplayFont2))
		font = QFont(sSansSerif, 6, QFont::Bold);
	m_ui.DisplayFont2TextLabel->setFont(font);
	m_ui.DisplayFont2TextLabel->setText(
		font.family() + ' ' + QString::number(font.pointSize()));

	if (m_pSetup->sMessagesFont.isEmpty()
		|| !font.fromString(m_pSetup->sMessagesFont))
		font = QFont("Monospace", 8);
	pal = m_ui.MessagesFontTextLabel->palette();
	pal.setColor(QPalette::Background, Qt::white);
	m_ui.MessagesFontTextLabel->setPalette(pal);
	m_ui.MessagesFontTextLabel->setFont(font);
	m_ui.MessagesFontTextLabel->setText(
		font.family() + ' ' + QString::number(font.pointSize()));

	if (m_pSetup->sConnectionsFont.isEmpty()
		|| !font.fromString(m_pSetup->sConnectionsFont))
		font = QFont(sSansSerif, 10);
	pal = m_ui.ConnectionsFontTextLabel->palette();
	pal.setColor(QPalette::Background, Qt::white);
	m_ui.ConnectionsFontTextLabel->setPalette(pal);
	m_ui.ConnectionsFontTextLabel->setFont(font);
	m_ui.ConnectionsFontTextLabel->setText(
		font.family() + ' ' + QString::number(font.pointSize()));

	// The main display shiny effect option.
	m_ui.DisplayEffectCheckBox->setChecked(m_pSetup->bDisplayEffect);
	toggleDisplayEffect(m_pSetup->bDisplayEffect);

	// Connections view icon size.
	m_ui.ConnectionsIconSizeComboBox->setCurrentIndex(
		m_pSetup->iConnectionsIconSize);

	// Messages limit option.
	m_ui.MessagesLimitCheckBox->setChecked(m_pSetup->bMessagesLimit);
	setComboBoxCurrentText(m_ui.MessagesLimitLinesComboBox,
		QString::number(m_pSetup->iMessagesLimitLines));

	// Other misc options...
	m_ui.StartJackCheckBox->setChecked(m_pSetup->bStartJack);
	m_ui.QueryCloseCheckBox->setChecked(m_pSetup->bQueryClose);
	m_ui.KeepOnTopCheckBox->setChecked(m_pSetup->bKeepOnTop);
	m_ui.SystemTrayCheckBox->setChecked(m_pSetup->bSystemTray);
	m_ui.DelayedSetupCheckBox->setChecked(m_pSetup->bDelayedSetup);
	m_ui.ServerConfigCheckBox->setChecked(m_pSetup->bServerConfig);
	setComboBoxCurrentText(m_ui.ServerConfigNameComboBox,
		m_pSetup->sServerConfigName);
	m_ui.ServerConfigTempCheckBox->setChecked(m_pSetup->bServerConfigTemp);
	m_ui.QueryShutdownCheckBox->setChecked(m_pSetup->bQueryShutdown);
	m_ui.AliasesEnabledCheckBox->setChecked(m_pSetup->bAliasesEnabled);
	m_ui.AliasesEditingCheckBox->setChecked(m_pSetup->bAliasesEditing);
	m_ui.LeftButtonsCheckBox->setChecked(!m_pSetup->bLeftButtons);
	m_ui.RightButtonsCheckBox->setChecked(!m_pSetup->bRightButtons);
	m_ui.TransportButtonsCheckBox->setChecked(!m_pSetup->bTransportButtons);
	m_ui.TextLabelsCheckBox->setChecked(!m_pSetup->bTextLabels);

#ifndef CONFIG_SYSTEM_TRAY
	m_ui.SystemTrayCheckBox->setChecked(false);
	m_ui.SystemTrayCheckBox->setEnabled(false);
#endif
#ifndef CONFIG_JACK_MIDI
	m_ui.MidiDriverComboBox->setCurrentIndex(0);
	m_ui.MidiDriverTextLabel->setEnabled(false);
	m_ui.MidiDriverComboBox->setEnabled(false);
#endif

	// Load preset list...
	resetPresets();
	setComboBoxCurrentText(m_ui.PresetComboBox,
		m_pSetup->sDefPreset);
	// Finally, load default settings...
	changePreset(m_ui.PresetComboBox->currentText());

	// We're clean now.
	m_iDirtySetup--;
	stabilizeForm();
}


void qjackctlSetupForm::changePreset ( const QString& sPreset )
{
	if (sPreset.isEmpty())
		return;

	// Load Settings...
	qjackctlPreset preset;
	if (m_pSetup->loadPreset(preset, sPreset)) {
		setComboBoxCurrentText(m_ui.ServerComboBox, preset.sServer);
		m_ui.RealtimeCheckBox->setChecked(preset.bRealtime);
		m_ui.SoftModeCheckBox->setChecked(preset.bSoftMode);
		m_ui.MonitorCheckBox->setChecked(preset.bMonitor);
		m_ui.ShortsCheckBox->setChecked(preset.bShorts);
		m_ui.NoMemLockCheckBox->setChecked(preset.bNoMemLock);
		m_ui.UnlockMemCheckBox->setChecked(preset.bUnlockMem);
		m_ui.HWMonCheckBox->setChecked(preset.bHWMon);
		m_ui.HWMeterCheckBox->setChecked(preset.bHWMeter);
		m_ui.IgnoreHWCheckBox->setChecked(preset.bIgnoreHW);
		m_ui.PrioritySpinBox->setValue(preset.iPriority);
		setComboBoxCurrentText(m_ui.FramesComboBox,
			QString::number(preset.iFrames));
		setComboBoxCurrentText(m_ui.SampleRateComboBox,
			QString::number(preset.iSampleRate));
		m_ui.PeriodsSpinBox->setValue(preset.iPeriods);
		setComboBoxCurrentText(m_ui.WordLengthComboBox,
			QString::number(preset.iWordLength));
		setComboBoxCurrentText(m_ui.WaitComboBox,
		QString::number(preset.iWait));
		m_ui.ChanSpinBox->setValue(preset.iChan);
		setComboBoxCurrentText(m_ui.DriverComboBox, preset.sDriver);
		setComboBoxCurrentText(m_ui.InterfaceComboBox,
			preset.sInterface.isEmpty()
				? m_pSetup->sDefPresetName
				: preset.sInterface);
		m_ui.AudioComboBox->setCurrentIndex(preset.iAudio);
		m_ui.DitherComboBox->setCurrentIndex(preset.iDither);
		setComboBoxCurrentText(m_ui.TimeoutComboBox,
			QString::number(preset.iTimeout));
		setComboBoxCurrentText(m_ui.InDeviceComboBox,
			preset.sInDevice.isEmpty()
				? m_pSetup->sDefPresetName
				: preset.sInDevice);
		setComboBoxCurrentText(m_ui.OutDeviceComboBox,
			preset.sOutDevice.isEmpty()
				? m_pSetup->sDefPresetName
				: preset.sOutDevice);
		m_ui.InChannelsSpinBox->setValue(preset.iInChannels);
		m_ui.OutChannelsSpinBox->setValue(preset.iOutChannels);
		m_ui.InLatencySpinBox->setValue(preset.iInLatency);
		m_ui.OutLatencySpinBox->setValue(preset.iOutLatency);
		m_ui.StartDelaySpinBox->setValue(preset.iStartDelay);
		m_ui.VerboseCheckBox->setChecked(preset.bVerbose);
		setComboBoxCurrentText(m_ui.PortMaxComboBox,
			QString::number(preset.iPortMax));
#ifdef CONFIG_JACK_MIDI
		setComboBoxCurrentText(m_ui.MidiDriverComboBox,
			preset.sMidiDriver);
#endif
		// Reset dirty flag.
		m_iDirtySettings = 0;
	}

	// Set current preset name..
	m_sPreset = sPreset;
}


bool qjackctlSetupForm::savePreset ( const QString& sPreset )
{
	if (sPreset.isEmpty())
		return false;

	// Unload settings.
	qjackctlPreset preset;
	preset.sServer      = m_ui.ServerComboBox->currentText();
	preset.bRealtime    = m_ui.RealtimeCheckBox->isChecked();
	preset.bSoftMode    = m_ui.SoftModeCheckBox->isChecked();
	preset.bMonitor     = m_ui.MonitorCheckBox->isChecked();
	preset.bShorts      = m_ui.ShortsCheckBox->isChecked();
	preset.bNoMemLock   = m_ui.NoMemLockCheckBox->isChecked();
	preset.bUnlockMem   = m_ui.UnlockMemCheckBox->isChecked();
	preset.bHWMon       = m_ui.HWMonCheckBox->isChecked();
	preset.bHWMeter     = m_ui.HWMeterCheckBox->isChecked();
	preset.bIgnoreHW    = m_ui.IgnoreHWCheckBox->isChecked();
	preset.iPriority    = m_ui.PrioritySpinBox->value();
	preset.iFrames      = m_ui.FramesComboBox->currentText().toInt();
	preset.iSampleRate  = m_ui.SampleRateComboBox->currentText().toInt();
	preset.iPeriods     = m_ui.PeriodsSpinBox->value();
	preset.iWordLength  = m_ui.WordLengthComboBox->currentText().toInt();
	preset.iWait        = m_ui.WaitComboBox->currentText().toInt();
	preset.iChan        = m_ui.ChanSpinBox->value();
	preset.sDriver      = m_ui.DriverComboBox->currentText();
	preset.sInterface   = m_ui.InterfaceComboBox->currentText();
	preset.iAudio       = m_ui.AudioComboBox->currentIndex();
	preset.iDither      = m_ui.DitherComboBox->currentIndex();
	preset.iTimeout     = m_ui.TimeoutComboBox->currentText().toInt();
	preset.sInDevice    = m_ui.InDeviceComboBox->currentText();
	preset.sOutDevice   = m_ui.OutDeviceComboBox->currentText();
	preset.iInChannels  = m_ui.InChannelsSpinBox->value();
	preset.iOutChannels = m_ui.OutChannelsSpinBox->value();
	preset.iInLatency   = m_ui.InLatencySpinBox->value();
	preset.iOutLatency  = m_ui.OutLatencySpinBox->value();
	preset.iStartDelay  = m_ui.StartDelaySpinBox->value();
	preset.bVerbose     = m_ui.VerboseCheckBox->isChecked();
	preset.iPortMax     = m_ui.PortMaxComboBox->currentText().toInt();
#ifdef CONFIG_JACK_MIDI
	preset.sMidiDriver  = m_ui.MidiDriverComboBox->currentText();
#endif
	if (preset.sInterface == m_pSetup->sDefPresetName)
		preset.sInterface.clear();
	if (preset.sInDevice == m_pSetup->sDefPresetName)
		preset.sInDevice.clear();
	if (preset.sOutDevice == m_pSetup->sDefPresetName)
		preset.sOutDevice.clear();
	m_pSetup->savePreset(preset, sPreset);

	return true;
}


bool qjackctlSetupForm::deletePreset ( const QString& sPreset )
{
	if (sPreset.isEmpty())
		return false;

	// Just remove the preset item...
	m_pSetup->deletePreset(sPreset);

	return true;
}


void qjackctlSetupForm::resetPresets (void)
{
	m_ui.PresetComboBox->clear();
	m_ui.PresetComboBox->addItems(m_pSetup->presets);
	m_ui.PresetComboBox->addItem(m_pSetup->sDefPresetName);
}


void qjackctlSetupForm::changeCurrentPreset ( const QString& sPreset )
{
	if (m_iDirtySetup > 0)
		return;

	// Check if there's any pending changes...
	if (m_iDirtySettings > 0 && !m_sPreset.isEmpty()) {
		switch (QMessageBox::warning(this,
			tr("Warning") + " - " QJACKCTL_SUBTITLE1,
			tr("Some settings have been changed:\n\n"
			"\"%1\"\n\nDo you want to save the changes?")
			.arg(m_sPreset),
			tr("Save"), tr("Discard"), tr("Cancel"))) {
		case 0: // Save...
			savePreset(m_sPreset);
			m_iDirtySetup++;
			resetPresets();
			setComboBoxCurrentText(m_ui.PresetComboBox, sPreset);
			m_iDirtySetup--;
		case 1: // Discard...
			m_iDirtySettings = 0;
			break;
		default:// Cancel...
			m_iDirtySetup++;
			resetPresets();
			setComboBoxCurrentText(m_ui.PresetComboBox, m_sPreset);
			m_iDirtySetup--;
			return;
		}
	}

	changePreset(sPreset);
	optionsChanged();
}

void qjackctlSetupForm::saveCurrentPreset (void)
{
	const QString sPreset = m_ui.PresetComboBox->currentText();

	if (savePreset(sPreset)) {
		// Reset preset combobox list.
		m_iDirtySetup++;
		resetPresets();
		setComboBoxCurrentText(m_ui.PresetComboBox, sPreset);
		m_iDirtySetup--;
		// Reset dirty flag.
		m_iDirtySettings = 0;
		stabilizeForm();
	}
}


void qjackctlSetupForm::deleteCurrentPreset (void)
{
	const QString sPreset = m_ui.PresetComboBox->currentText();

	// Try to prompt user if he/she really wants this...
	if (QMessageBox::warning(this,
		tr("Warning") + " - " QJACKCTL_SUBTITLE1,
		tr("Delete preset:\n\n"
		"\"%1\"\n\nAre you sure?")
		.arg(sPreset),
		tr("OK"), tr("Cancel")) > 0)
		return;

	if (deletePreset(sPreset)) {
		// Reset preset combobox list,
		// and load a new available preset..
		m_iDirtySetup++;
		int iItem = m_ui.PresetComboBox->currentIndex();
		resetPresets();
		m_ui.PresetComboBox->setCurrentIndex(iItem);
		changePreset(m_ui.PresetComboBox->currentText());
		m_iDirtySetup--;
		// Take care that maybe it was the default one...
		if (m_pSetup->sDefPreset == sPreset)
			m_pSetup->sDefPreset = m_sPreset;
		// Make this stable now.
		optionsChanged();
	}
}


void qjackctlSetupForm::computeLatency (void)
{
	float lat = 0.0;
	int p = m_ui.FramesComboBox->currentText().toInt();
	int r = m_ui.SampleRateComboBox->currentText().toInt();
	int n = m_ui.PeriodsSpinBox->value();
	if (r > 0)
		lat = (float) (1000.0 * p * n) / (float) r;
	if (lat > 0.0)
		m_ui.LatencyTextValue->setText(QString::number(lat, 'g', 3) + " " + tr("msec"));
	else
		m_ui.LatencyTextValue->setText(tr("n/a"));

	m_ui.OkPushButton->setEnabled(m_iDirtySettings > 0 || m_iDirtyOptions > 0);
}


void qjackctlSetupForm::changeDriverAudio ( const QString& sDriver, int iAudio )
{
	bool bOss        = (sDriver == "oss");
	bool bAlsa       = (sDriver == "alsa");
	bool bCoreaudio  = (sDriver == "coreaudio");
	bool bFreebob    = (sDriver == "freebob");
	bool bInEnabled  = false;
	bool bOutEnabled = false;

	switch (iAudio) {
	case QJACKCTL_DUPLEX:
		bInEnabled  = (bOss || bAlsa || bCoreaudio);
		bOutEnabled = (bOss || bAlsa || bCoreaudio);
		break;
	case QJACKCTL_CAPTURE:
		bInEnabled  = (bOss || bCoreaudio);
		break;
	case QJACKCTL_PLAYBACK:
		bOutEnabled = (bOss || bCoreaudio);
		break;
	}

	m_ui.InDeviceTextLabel->setEnabled(bInEnabled && (bAlsa || bOss));
	m_ui.InDeviceComboBox->setEnabled(bInEnabled && (bAlsa || bOss));
	m_ui.InDeviceToolButton->setEnabled(bInEnabled && (bAlsa || bOss));
	m_ui.OutDeviceTextLabel->setEnabled(bOutEnabled && (bAlsa || bOss));
	m_ui.OutDeviceComboBox->setEnabled(bOutEnabled && (bAlsa || bOss));
	m_ui.OutDeviceToolButton->setEnabled(bOutEnabled && (bAlsa || bOss));

	m_ui.InChannelsTextLabel->setEnabled(bInEnabled
		|| (bAlsa && iAudio != QJACKCTL_PLAYBACK));
	m_ui.InChannelsSpinBox->setEnabled(bInEnabled
		|| (bAlsa && iAudio != QJACKCTL_PLAYBACK));
	m_ui.OutChannelsTextLabel->setEnabled(bOutEnabled
		|| (bAlsa && iAudio != QJACKCTL_CAPTURE));
	m_ui.OutChannelsSpinBox->setEnabled(bOutEnabled
		|| (bAlsa && iAudio != QJACKCTL_CAPTURE));

	m_ui.InLatencyTextLabel->setEnabled(bInEnabled
		|| ((bAlsa || bFreebob) && iAudio != QJACKCTL_PLAYBACK));
	m_ui.InLatencySpinBox->setEnabled(bInEnabled
		|| ((bAlsa || bFreebob) && iAudio != QJACKCTL_PLAYBACK));
	m_ui.OutLatencyTextLabel->setEnabled(bOutEnabled
		|| ((bAlsa || bFreebob) && iAudio != QJACKCTL_CAPTURE));
	m_ui.OutLatencySpinBox->setEnabled(bOutEnabled
		|| ((bAlsa || bFreebob) && iAudio != QJACKCTL_CAPTURE));

	computeLatency();
}


void qjackctlSetupForm::changeAudio ( int iAudio )
{
	changeDriverAudio(m_ui.DriverComboBox->currentText(), iAudio);
}


void qjackctlSetupForm::changeDriver ( const QString& sDriver )
{
	changeDriverUpdate(sDriver, true);
}


void qjackctlSetupForm::changeDriverUpdate ( const QString& sDriver, bool bUpdate )
{
	bool bDummy     = (sDriver == "dummy");
	bool bOss       = (sDriver == "oss");
	bool bAlsa      = (sDriver == "alsa");
	bool bPortaudio = (sDriver == "portaudio");
	bool bCoreaudio = (sDriver == "coreaudio");
	bool bFreebob   = (sDriver == "freebob");

	m_ui.NoMemLockCheckBox->setEnabled(!bCoreaudio);
	m_ui.UnlockMemCheckBox->setEnabled(!bCoreaudio
		&& !m_ui.NoMemLockCheckBox->isChecked());

	m_ui.SoftModeCheckBox->setEnabled(bAlsa);
	m_ui.MonitorCheckBox->setEnabled(bAlsa);
	m_ui.ShortsCheckBox->setEnabled(bAlsa);
	m_ui.HWMonCheckBox->setEnabled(bAlsa);
	m_ui.HWMeterCheckBox->setEnabled(bAlsa);

	m_ui.IgnoreHWCheckBox->setEnabled(bOss);

#ifdef CONFIG_COREAUDIO
	m_ui.PriorityTextLabel->setEnabled(false);
	m_ui.PrioritySpinBox->setEnabled(false);
#else
	bool bPriorityEnabled = m_ui.RealtimeCheckBox->isChecked() && !bCoreaudio;
	m_ui.PriorityTextLabel->setEnabled(bPriorityEnabled);
	m_ui.PrioritySpinBox->setEnabled(bPriorityEnabled);
#endif

	m_ui.PeriodsTextLabel->setEnabled(bAlsa || bOss || bFreebob);
	m_ui.PeriodsSpinBox->setEnabled(bAlsa || bOss || bFreebob);

	if (bUpdate && bFreebob && m_ui.PeriodsSpinBox->value() < 3)
		m_ui.PeriodsSpinBox->setValue(3);

	m_ui.WordLengthTextLabel->setEnabled(bOss);
	m_ui.WordLengthComboBox->setEnabled(bOss);

	m_ui.WaitTextLabel->setEnabled(bDummy);
	m_ui.WaitComboBox->setEnabled(bDummy);

	m_ui.ChanTextLabel->setEnabled(bPortaudio);
	m_ui.ChanSpinBox->setEnabled(bPortaudio);

	int  iAudio   = m_ui.AudioComboBox->currentIndex();
	bool bEnabled = bAlsa;
	if (bEnabled && iAudio == QJACKCTL_DUPLEX) {
		const QString& sInDevice  = m_ui.InDeviceComboBox->currentText();
		const QString& sOutDevice = m_ui.OutDeviceComboBox->currentText();
		bEnabled = (sInDevice.isEmpty()  || sInDevice  == m_pSetup->sDefPresetName ||
					sOutDevice.isEmpty() || sOutDevice == m_pSetup->sDefPresetName);
	}
	m_ui.InterfaceTextLabel->setEnabled(bEnabled || bCoreaudio || bFreebob);
	m_ui.InterfaceComboBox->setEnabled(bEnabled || bCoreaudio || bFreebob);
	m_ui.InterfaceToolButton->setEnabled(bEnabled || bCoreaudio);

	m_ui.DitherTextLabel->setEnabled(bAlsa || bPortaudio);
	m_ui.DitherComboBox->setEnabled(bAlsa || bPortaudio);

#ifdef CONFIG_JACK_MIDI
	m_ui.MidiDriverTextLabel->setEnabled(bAlsa);
	m_ui.MidiDriverComboBox->setEnabled(bAlsa);
#endif

	changeDriverAudio(sDriver, iAudio);
}


// Stabilize current form state.
void qjackctlSetupForm::stabilizeForm (void)
{
	QString sPreset = m_ui.PresetComboBox->currentText();
	if (!sPreset.isEmpty()) {
		bool bPreset = (m_pSetup->presets.contains(sPreset));
		m_ui.PresetSavePushButton->setEnabled(m_iDirtySettings > 0
			|| (!bPreset && sPreset != m_pSetup->sDefPresetName));
		m_ui.PresetDeletePushButton->setEnabled(bPreset);
	} else {
		m_ui.PresetSavePushButton->setEnabled(false);
		m_ui.PresetDeletePushButton->setEnabled(false);
	}

	bool bEnabled = m_ui.StartupScriptCheckBox->isChecked();
	m_ui.StartupScriptShellComboBox->setEnabled(bEnabled);
	m_ui.StartupScriptSymbolToolButton->setEnabled(bEnabled);
	m_ui.StartupScriptBrowseToolButton->setEnabled(bEnabled);

	bEnabled = m_ui.PostStartupScriptCheckBox->isChecked();
	m_ui.PostStartupScriptShellComboBox->setEnabled(bEnabled);
	m_ui.PostStartupScriptSymbolToolButton->setEnabled(bEnabled);
	m_ui.PostStartupScriptBrowseToolButton->setEnabled(bEnabled);

	bEnabled = m_ui.ShutdownScriptCheckBox->isChecked();
	m_ui.ShutdownScriptShellComboBox->setEnabled(bEnabled);
	m_ui.ShutdownScriptSymbolToolButton->setEnabled(bEnabled);
	m_ui.ShutdownScriptBrowseToolButton->setEnabled(bEnabled);

	bEnabled = m_ui.PostShutdownScriptCheckBox->isChecked();
	m_ui.PostShutdownScriptShellComboBox->setEnabled(bEnabled);
	m_ui.PostShutdownScriptSymbolToolButton->setEnabled(bEnabled);
	m_ui.PostShutdownScriptBrowseToolButton->setEnabled(bEnabled);

	bEnabled = m_ui.StdoutCaptureCheckBox->isChecked();
	m_ui.XrunRegexTextLabel->setEnabled(bEnabled);
	m_ui.XrunRegexComboBox->setEnabled(bEnabled);
	m_ui.XrunIgnoreFirstCheckBox->setEnabled(bEnabled);

	bEnabled = m_ui.ActivePatchbayCheckBox->isChecked();
	m_ui.ActivePatchbayPathComboBox->setEnabled(bEnabled);
	m_ui.ActivePatchbayPathToolButton->setEnabled(bEnabled);

	m_ui.TimeRefreshComboBox->setEnabled(
		m_ui.AutoRefreshCheckBox->isChecked());
	m_ui.MessagesLimitLinesComboBox->setEnabled(
		m_ui.MessagesLimitCheckBox->isChecked());

	bEnabled = m_ui.ServerConfigCheckBox->isChecked();
	m_ui.ServerConfigNameComboBox->setEnabled(bEnabled);
	m_ui.ServerConfigTempCheckBox->setEnabled(bEnabled);

	m_ui.AliasesEditingCheckBox->setEnabled(
		m_ui.AliasesEnabledCheckBox->isChecked());

	m_ui.TransportButtonsCheckBox->setEnabled(
		m_ui.LeftButtonsCheckBox->isChecked());

	changeDriverUpdate(m_ui.DriverComboBox->currentText(), false);
}


#ifdef CONFIG_COREAUDIO
// borrowed from jackpilot source
static OSStatus getTotalChannels( AudioDeviceID device,
	UInt32* channelCount, Boolean isInput )
{
	OSStatus            err = noErr;
	UInt32              outSize;
	Boolean             outWritable;
	AudioBufferList*    bufferList = 0;
	AudioStreamID*      streamList = 0;
	size_t              i, numStream;

	err = AudioDeviceGetPropertyInfo(device, 0, isInput,
		kAudioDevicePropertyStreams, &outSize, &outWritable);
	if (err == noErr) {
		streamList = (AudioStreamID*)malloc(outSize);
		numStream = outSize/sizeof(AudioStreamID);
		//JPLog("getTotalChannels device stream number %ld %ld\n", device, numStream);
		err = AudioDeviceGetProperty(device, 0, isInput,
			kAudioDevicePropertyStreams, &outSize, streamList);
		if (err == noErr) {
			AudioStreamBasicDescription streamDesc;
			outSize = sizeof(AudioStreamBasicDescription);
			for (i = 0; i < numStream; i++) {
				err = AudioStreamGetProperty(streamList[i], 0,
					kAudioDevicePropertyStreamFormat, &outSize, &streamDesc);
				//JPLog("getTotalChannels streamDesc mFormatFlags %ld mChannelsPerFrame %ld\n", streamDesc.mFormatFlags, streamDesc.mChannelsPerFrame);
			}
		}
	}

	*channelCount = 0;
	err = AudioDeviceGetPropertyInfo(device, 0, isInput,
		kAudioDevicePropertyStreamConfiguration, &outSize, &outWritable);
	if (err == noErr) {
		bufferList = (AudioBufferList*)malloc(outSize);
		err = AudioDeviceGetProperty(device, 0, isInput,
			kAudioDevicePropertyStreamConfiguration, &outSize, bufferList);
		if (err == noErr) {								
			for (i = 0; i < bufferList->mNumberBuffers; i++) 
				*channelCount += bufferList->mBuffers[i].mNumberChannels;
		}
	}

	if (streamList) 
		free(streamList);
	if (bufferList) 
		free(bufferList);	

	return (err);
}

static OSStatus getDeviceUIDFromID( AudioDeviceID id,
	char *name, UInt32 nsize )
{
	UInt32 size = sizeof(CFStringRef);
	CFStringRef UI;
	OSStatus res = AudioDeviceGetProperty(id, 0, false,
		kAudioDevicePropertyDeviceUID, &size, &UI);
	if (res == noErr) 
		CFStringGetCString(UI,name,nsize,CFStringGetSystemEncoding());
	CFRelease(UI);
	return res;
}

#endif // CONFIG_COREAUDIO


// Device selection menu executive.
void qjackctlSetupForm::deviceMenu( QLineEdit *pLineEdit,
	QToolButton *pToolButton, int iAudio )
{
	// FIXME: Only valid for ALSA and OSS devices,
	// for the time being... and also CoreAudio ones too.
	const QString& sDriver = m_ui.DriverComboBox->currentText();
	bool bAlsa      = (sDriver == "alsa");
	bool bOss       = (sDriver == "oss");
#ifdef CONFIG_COREAUDIO
	bool bCoreaudio = (sDriver == "coreaudio");
	std::map<QString,AudioDeviceID> coreaudioIdMap;
#endif
	QString sCurName = pLineEdit->text();
	QString sName, sSubName;
	QString sText;
	
	QMenu menu(this);
	QAction *pAction;

	int iCards = 0;

	if (bAlsa) {
#ifdef CONFIG_ALSA_SEQ
		// Enumerate the ALSA cards and PCM harfware devices...
		snd_ctl_t *handle;
		snd_ctl_card_info_t *info;
		snd_pcm_info_t *pcminfo;
		snd_ctl_card_info_alloca(&info);
		snd_pcm_info_alloca(&pcminfo);
		int iCard = -1;
		while (snd_card_next(&iCard) >= 0 && iCard >= 0) {
			sName = "hw:" + QString::number(iCard);
			if (snd_ctl_open(&handle, sName.toUtf8().constData(), 0) >= 0
				&& snd_ctl_card_info(handle, info) >= 0) {
				if (iCards > 0)
					menu.addSeparator();
				sText  = sName + '\t';
				sText += snd_ctl_card_info_get_name(info);
				pAction = menu.addAction(sText);
				pAction->setCheckable(true);
				pAction->setChecked(sCurName == sName);
				int iDevice = -1;
				while (snd_ctl_pcm_next_device(handle, &iDevice) >= 0
					&& iDevice >= 0) {
					snd_pcm_info_set_device(pcminfo, iDevice);
					snd_pcm_info_set_subdevice(pcminfo, 0);
					snd_pcm_info_set_stream(pcminfo,
						iAudio == QJACKCTL_CAPTURE ?
							SND_PCM_STREAM_CAPTURE :
							SND_PCM_STREAM_PLAYBACK);
					if (snd_ctl_pcm_info(handle, pcminfo) >= 0) {
						sSubName = sName + ',' + QString::number(iDevice);
						sText  = sSubName + '\t';
						sText += snd_pcm_info_get_name(pcminfo);
						pAction = menu.addAction(sText);
						pAction->setCheckable(true);
						pAction->setChecked(sCurName == sSubName);
					}
				}
				snd_ctl_close(handle);
				++iCards;
			}
		}
#endif 	// CONFIG_ALSA_SEQ
	}	// Enumerate the OSS Audio devices...
	else
	if (bOss) {
		QFile file("/dev/sndstat");
		if (file.open(QIODevice::ReadOnly)) {
			QTextStream stream(&file);
			QString sLine;
			bool bAudioDevices = false;
			QRegExp rxHeader("Audio devices.*", Qt::CaseInsensitive);
			QRegExp rxDevice("([0-9]+):[ ]+(.*)");
			while (!stream.atEnd()) {
				sLine = stream.readLine();
				if (bAudioDevices) {
					if (rxDevice.exactMatch(sLine)) {
						sName = "/dev/dsp" + rxDevice.cap(1);
						sText = sName + '\t' + rxDevice.cap(2);
						pAction = menu.addAction(sText);
						pAction->setCheckable(true);
						pAction->setChecked(sCurName == sName);
						++iCards;
					}
					else break;
				}
				else if (rxHeader.exactMatch(sLine))
					bAudioDevices = true;
			}
			file.close();
		}
	}
#ifdef CONFIG_COREAUDIO
	else if (bCoreaudio) {
		// Find out how many Core Audio devices are there, if any...
		// (code snippet gently "borrowed" from St?hane Letz jackdmp;)
		OSStatus err;
		Boolean isWritable;
		UInt32 outSize = sizeof(isWritable);
		err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices,
				&outSize, &isWritable);
		if (err == noErr) {
			// Calculate the number of device available...
			int numCoreDevices = outSize / sizeof(AudioDeviceID);
			// Make space for the devices we are about to get...
			AudioDeviceID *coreDeviceIDs = new AudioDeviceID [numCoreDevices];
			err = AudioHardwareGetProperty(kAudioHardwarePropertyDevices,
					&outSize, (void *) coreDeviceIDs);
			if (err == noErr) {
				// Look for the CoreAudio device name...
				char coreDeviceName[256];
				UInt32 nameSize = 256;
				for (int i = 0; i < numCoreDevices; i++) {
					err = AudioDeviceGetPropertyInfo(coreDeviceIDs[i],
							0, true, kAudioDevicePropertyDeviceName,
							&outSize, &isWritable);
					if (err == noErr) {
						err = AudioDeviceGetProperty(coreDeviceIDs[i],
								0, true, kAudioDevicePropertyDeviceName,
								&nameSize, (void *) coreDeviceName);
						if (err == noErr) {
							char drivername[128];
							UInt32 dnsize = 128;
							// this returns the unique id for the device
							// that must be used on the commandline for jack
							if (getDeviceUIDFromID(coreDeviceIDs[i],
								drivername, dnsize) == noErr) {
								sName = drivername;
							} else {
								sName = "Error";
							}
							coreaudioIdMap[sName] = coreDeviceIDs[i];
							// TODO: hide this ugly ID from the user,
							// only show human readable name
							// humanreadable \t UID
							sText = QString(coreDeviceName) + '\t' + sName;
							pAction = menu.addAction(sText);
							pAction->setCheckable(true);
							pAction->setChecked(sCurName == sName);
							++iCards;
						}
					}
				}
			}
			delete [] coreDeviceIDs;
		}
	}
#endif 	// CONFIG_COREAUDIO

	// There's always the default device...
	if (iCards > 0)
		menu.addSeparator();
	pAction = menu.addAction(m_pSetup->sDefPresetName);
	pAction->setCheckable(true);
	pAction->setChecked(sCurName == m_pSetup->sDefPresetName);

	// Show the device menu and read selection...
	pAction = menu.exec(pToolButton->mapToGlobal(QPoint(0,0)));
	if (pAction) {
		sText = pAction->text();
		int iTabPos = sText.indexOf('\t');
		if (iTabPos >= 0) {
#ifndef CONFIG_COREAUDIO
			pLineEdit->setText(sText.left(iTabPos));
#else
			// for OSX, figure out the device's channel counts and set the combos.
			// this might be too difficult for the user to determine themselves
			// and jack won't start if the values exceed the actual.
			// we now use the second tab delimited field as the value to
			// put in the combo
			pLineEdit->setText(sText.mid(iTabPos + 1));
			std::map<QString,AudioDeviceID>::iterator found
				= coreaudioIdMap.find(sText.mid(iTabPos+1));
			if (found != coreaudioIdMap.end()) {
				AudioDeviceID devid = found->second;
				UInt32 chans;
				if (getTotalChannels(devid, &chans, true) == noErr) {
					//InChannelsSpinBox->setMaxValue(chans);
					m_ui.InChannelsSpinBox->setValue(chans);
				}
				if (getTotalChannels(devid, &chans, false) == noErr) {
					//OutChannelsSpinBox->setMaxValue(chans);
					m_ui.OutChannelsSpinBox->setValue(chans);
				}
			}
#endif
		} else {
			pLineEdit->setText(sText);
		}
	//  settingsChanged();
	}
}


// Interface device selection menu.
void qjackctlSetupForm::selectInterface (void)
{
	deviceMenu(m_ui.InterfaceComboBox->lineEdit(),
		m_ui.InterfaceToolButton, m_ui.AudioComboBox->currentIndex());
}


// Input device selection menu.
void qjackctlSetupForm::selectInDevice (void)
{
	deviceMenu(m_ui.InDeviceComboBox->lineEdit(),
		m_ui.InDeviceToolButton, QJACKCTL_CAPTURE);
}


// Output device selection menu.
void qjackctlSetupForm::selectOutDevice (void)
{
	deviceMenu(m_ui.OutDeviceComboBox->lineEdit(),
		m_ui.OutDeviceToolButton, QJACKCTL_PLAYBACK);
}


// Meta-symbol menu executive.
void qjackctlSetupForm::symbolMenu( QLineEdit *pLineEdit,
	QToolButton *pToolButton )
{
	const QString s = "  ";

	QMenu menu(this);

	menu.addAction("%P" + s + tr("&Preset Name"));
	menu.addSeparator();
	menu.addAction("%s" + s + tr("&Server Path"));
	menu.addAction("%d" + s + tr("&Driver"));
	menu.addAction("%i" + s + tr("&Interface"));
	menu.addSeparator();
	menu.addAction("%r" + s + tr("Sample &Rate"));
	menu.addAction("%p" + s + tr("&Frames/Period"));
	menu.addAction("%n" + s + tr("Periods/&Buffer"));

	QAction *pAction = menu.exec(pToolButton->mapToGlobal(QPoint(0,0)));
	if (pAction) {
		const QString sText = pAction->text();
		int iMetaChar = sText.indexOf('%');
		if (iMetaChar >= 0) {
			pLineEdit->insert('%' + sText[iMetaChar + 1]);
		//  optionsChanged();
		}
	}
}


// Startup script meta-symbol button slot.
void qjackctlSetupForm::symbolStartupScript (void)
{
	symbolMenu(m_ui.StartupScriptShellComboBox->lineEdit(),
		m_ui.StartupScriptSymbolToolButton);
}

// Post-startup script meta-symbol button slot.
void qjackctlSetupForm::symbolPostStartupScript (void)
{
	symbolMenu(m_ui.PostStartupScriptShellComboBox->lineEdit(),
		m_ui.PostStartupScriptSymbolToolButton);
}

// Shutdown script meta-symbol button slot.
void qjackctlSetupForm::symbolShutdownScript (void)
{
	symbolMenu(m_ui.ShutdownScriptShellComboBox->lineEdit(),
		m_ui.ShutdownScriptSymbolToolButton);
}

// Post-shutdown script meta-symbol button slot.
void qjackctlSetupForm::symbolPostShutdownScript (void)
{
	symbolMenu(m_ui.PostShutdownScriptShellComboBox->lineEdit(),
		m_ui.PostShutdownScriptSymbolToolButton);
}


// Startup script browse slot.
void qjackctlSetupForm::browseStartupScript()
{
	QString sFileName = QFileDialog::getOpenFileName(
		this,												// Parent.
		tr("Startup Script"),								// Caption.
		m_ui.StartupScriptShellComboBox->currentText() 		// Start here.
	);

	if (!sFileName.isEmpty()) {
		setComboBoxCurrentText(m_ui.StartupScriptShellComboBox, sFileName);
		m_ui.StartupScriptShellComboBox->setFocus();
		optionsChanged();
	}
}


// Post-startup script browse slot.
void qjackctlSetupForm::browsePostStartupScript()
{
	QString sFileName = QFileDialog::getOpenFileName(
		this,												// Parent.
		tr("Post-Startup Script"),							// Caption.
		m_ui.PostStartupScriptShellComboBox->currentText() 	// Start here.
	);

	if (!sFileName.isEmpty()) {
		setComboBoxCurrentText(m_ui.PostStartupScriptShellComboBox, sFileName);
		m_ui.PostStartupScriptShellComboBox->setFocus();
		optionsChanged();
	}
}


// Shutdown script browse slot.
void qjackctlSetupForm::browseShutdownScript()
{
	QString sFileName = QFileDialog::getOpenFileName(
		this,												// Parent.
		tr("Shutdown Script"),								// Caption.
		m_ui.ShutdownScriptShellComboBox->currentText() 	// Start here.
	);

	if (!sFileName.isEmpty()) {
		setComboBoxCurrentText(m_ui.ShutdownScriptShellComboBox, sFileName);
		m_ui.ShutdownScriptShellComboBox->setFocus();
		optionsChanged();
	}
}


// Post-shutdown script browse slot.
void qjackctlSetupForm::browsePostShutdownScript()
{
	QString sFileName = QFileDialog::getOpenFileName(
		this,												// Parent.
		tr("Post-Shutdown Script"),							// Caption.
		m_ui.PostShutdownScriptShellComboBox->currentText() // Start here.
	);

	if (!sFileName.isEmpty()) {
		setComboBoxCurrentText(m_ui.PostShutdownScriptShellComboBox, sFileName);
		m_ui.PostShutdownScriptShellComboBox->setFocus();
		optionsChanged();
	}
}


// Active Patchbay path browse slot.
void qjackctlSetupForm::browseActivePatchbayPath()
{
	QString sFileName = QFileDialog::getOpenFileName(
		this,											// Parent.
		tr("Active Patchbay Definition"),				// Caption.
		m_ui.ActivePatchbayPathComboBox->currentText(),	// Start here.
		tr("Patchbay Definition files") + " (*.xml)"	// Filter (XML files)
	);

	if (!sFileName.isEmpty()) {
		setComboBoxCurrentText(m_ui.ActivePatchbayPathComboBox, sFileName);
		m_ui.ActivePatchbayPathComboBox->setFocus();
		optionsChanged();
	}
}


// The display font 1 (big time) selection dialog.
void qjackctlSetupForm::chooseDisplayFont1()
{
	bool  bOk  = false;
	QFont font = QFontDialog::getFont(&bOk,
		m_ui.DisplayFont1TextLabel->font(), this);
	if (bOk) {
		m_ui.DisplayFont1TextLabel->setFont(font);
		m_ui.DisplayFont1TextLabel->setText(font.family()
			+ ' ' + QString::number(font.pointSize()));
		optionsChanged();
	}
}


// The display font 2 (normal time et al.) selection dialog.
void qjackctlSetupForm::chooseDisplayFont2()
{
	bool  bOk  = false;
	QFont font = QFontDialog::getFont(&bOk,
		m_ui.DisplayFont2TextLabel->font(), this);
	if (bOk) {
		m_ui.DisplayFont2TextLabel->setFont(font);
		m_ui.DisplayFont2TextLabel->setText(font.family()
			+ ' ' + QString::number(font.pointSize()));
		optionsChanged();
	}
}


// The channel display effect demo changer.
void qjackctlSetupForm::toggleDisplayEffect ( bool bOn )
{
	QPalette pal;
	pal.setColor(QPalette::Foreground, Qt::green);
	if (bOn) {
		QPixmap pm(":/icons/displaybg1.png");
		pal.setBrush(QPalette::Background, QBrush(pm));
	} else {
		pal.setColor(QPalette::Background, Qt::black);
	}
	m_ui.DisplayFont1TextLabel->setPalette(pal);
	m_ui.DisplayFont2TextLabel->setPalette(pal);
	optionsChanged();
}


// The messages font selection dialog.
void qjackctlSetupForm::chooseMessagesFont (void)
{
	bool  bOk  = false;
	QFont font = QFontDialog::getFont(&bOk,
		m_ui.MessagesFontTextLabel->font(), this);
	if (bOk) {
		m_ui.MessagesFontTextLabel->setFont(font);
		m_ui.MessagesFontTextLabel->setText(font.family()
			+ ' ' + QString::number(font.pointSize()));
		optionsChanged();
	}
}


// The connections font selection dialog.
void qjackctlSetupForm::chooseConnectionsFont (void)
{
	bool  bOk  = false;
	QFont font = QFontDialog::getFont(&bOk,
		m_ui.ConnectionsFontTextLabel->font(), this);
	if (bOk) {
		m_ui.ConnectionsFontTextLabel->setFont(font);
		m_ui.ConnectionsFontTextLabel->setText(font.family()
			+ ' ' + QString::number(font.pointSize()));
		optionsChanged();
	}
}


// Mark that some server preset settings have changed.
void qjackctlSetupForm::settingsChanged (void)
{
	if (m_iDirtySetup > 0)
		return;

	m_iDirtySettings++;
	stabilizeForm();
}

// Mark that some program options have changed.
void qjackctlSetupForm::optionsChanged (void)
{
	if (m_iDirtySetup > 0)
		return;

	m_iDirtyOptions++;
	stabilizeForm();
}


// Accept settings (OK button slot).
void qjackctlSetupForm::accept (void)
{
	if (m_iDirtySettings > 0 || m_iDirtyOptions > 0) {
		// Save current preset selection.
		m_pSetup->sDefPreset = m_ui.PresetComboBox->currentText();
		// Always save current settings...
		savePreset(m_pSetup->sDefPreset);
		// Save Options...
		m_pSetup->bStartupScript           = m_ui.StartupScriptCheckBox->isChecked();
		m_pSetup->sStartupScriptShell      = m_ui.StartupScriptShellComboBox->currentText();
		m_pSetup->bPostStartupScript       = m_ui.PostStartupScriptCheckBox->isChecked();
		m_pSetup->sPostStartupScriptShell  = m_ui.PostStartupScriptShellComboBox->currentText();
		m_pSetup->bShutdownScript          = m_ui.ShutdownScriptCheckBox->isChecked();
		m_pSetup->sShutdownScriptShell     = m_ui.ShutdownScriptShellComboBox->currentText();
		m_pSetup->bPostShutdownScript      = m_ui.PostShutdownScriptCheckBox->isChecked();
		m_pSetup->sPostShutdownScriptShell = m_ui.PostShutdownScriptShellComboBox->currentText();
		m_pSetup->bStdoutCapture           = m_ui.StdoutCaptureCheckBox->isChecked();
		m_pSetup->sXrunRegex               = m_ui.XrunRegexComboBox->currentText();
		m_pSetup->bXrunIgnoreFirst         = m_ui.XrunIgnoreFirstCheckBox->isChecked();
		m_pSetup->bActivePatchbay          = m_ui.ActivePatchbayCheckBox->isChecked();
		m_pSetup->sActivePatchbayPath      = m_ui.ActivePatchbayPathComboBox->currentText();
		m_pSetup->bAutoRefresh             = m_ui.AutoRefreshCheckBox->isChecked();
		m_pSetup->iTimeRefresh             = m_ui.TimeRefreshComboBox->currentText().toInt();
		m_pSetup->bBezierLines             = m_ui.BezierLinesCheckBox->isChecked();
		// Save Defaults...
		m_pSetup->iTimeDisplay             = m_pTimeDisplayButtonGroup->checkedId();
		m_pSetup->iTimeFormat              = m_ui.TimeFormatComboBox->currentIndex();
		m_pSetup->sMessagesFont            = m_ui.MessagesFontTextLabel->font().toString();
		m_pSetup->bMessagesLimit           = m_ui.MessagesLimitCheckBox->isChecked();
		m_pSetup->iMessagesLimitLines      = m_ui.MessagesLimitLinesComboBox->currentText().toInt();
		m_pSetup->sDisplayFont1            = m_ui.DisplayFont1TextLabel->font().toString();
		m_pSetup->sDisplayFont2            = m_ui.DisplayFont2TextLabel->font().toString();
		m_pSetup->bDisplayEffect           = m_ui.DisplayEffectCheckBox->isChecked();
		m_pSetup->iConnectionsIconSize     = m_ui.ConnectionsIconSizeComboBox->currentIndex();
		m_pSetup->sConnectionsFont         = m_ui.ConnectionsFontTextLabel->font().toString();
		m_pSetup->bStartJack               = m_ui.StartJackCheckBox->isChecked();
		m_pSetup->bQueryClose              = m_ui.QueryCloseCheckBox->isChecked();
		m_pSetup->bKeepOnTop               = m_ui.KeepOnTopCheckBox->isChecked();
		m_pSetup->bSystemTray              = m_ui.SystemTrayCheckBox->isChecked();
		m_pSetup->bDelayedSetup            = m_ui.DelayedSetupCheckBox->isChecked();
		m_pSetup->bServerConfig            = m_ui.ServerConfigCheckBox->isChecked();
		m_pSetup->sServerConfigName        = m_ui.ServerConfigNameComboBox->currentText();
		m_pSetup->bServerConfigTemp        = m_ui.ServerConfigTempCheckBox->isChecked();
		m_pSetup->bQueryShutdown           = m_ui.QueryShutdownCheckBox->isChecked();
		m_pSetup->bAliasesEnabled          = m_ui.AliasesEnabledCheckBox->isChecked();
		m_pSetup->bAliasesEditing          = m_ui.AliasesEditingCheckBox->isChecked();
		m_pSetup->bLeftButtons             = !m_ui.LeftButtonsCheckBox->isChecked();
		m_pSetup->bRightButtons            = !m_ui.RightButtonsCheckBox->isChecked();
		m_pSetup->bTransportButtons        = !m_ui.TransportButtonsCheckBox->isChecked();
		m_pSetup->bTextLabels              = !m_ui.TextLabelsCheckBox->isChecked();
	}

	// Save combobox history...
	m_pSetup->saveComboBoxHistory(m_ui.ServerComboBox);
	m_pSetup->saveComboBoxHistory(m_ui.InterfaceComboBox);
	m_pSetup->saveComboBoxHistory(m_ui.InDeviceComboBox);
	m_pSetup->saveComboBoxHistory(m_ui.OutDeviceComboBox);
	m_pSetup->saveComboBoxHistory(m_ui.StartupScriptShellComboBox);
	m_pSetup->saveComboBoxHistory(m_ui.PostStartupScriptShellComboBox);
	m_pSetup->saveComboBoxHistory(m_ui.ShutdownScriptShellComboBox);
	m_pSetup->saveComboBoxHistory(m_ui.PostShutdownScriptShellComboBox);
	m_pSetup->saveComboBoxHistory(m_ui.XrunRegexComboBox);
	m_pSetup->saveComboBoxHistory(m_ui.ActivePatchbayPathComboBox);
	m_pSetup->saveComboBoxHistory(m_ui.ServerConfigNameComboBox);

	// Just go with dialog acceptance.
	QDialog::accept();
}


// Reject settings (Cancel button slot).
void qjackctlSetupForm::reject (void)
{
	bool bReject = true;

	// Check if there's any pending changes...
	if (m_iDirtySettings > 0 || m_iDirtyOptions > 0) {
		switch (QMessageBox::warning(this,
			tr("Warning") + " - " QJACKCTL_SUBTITLE1,
			tr("Some settings have been changed.\n\n"
			"Do you want to apply the changes?"),
			tr("Apply"), tr("Discard"), tr("Cancel"))) {
		case 0:     // Apply...
			accept();
			return;
		case 1:     // Discard
			break;
		default:    // Cancel.
			bReject = false;
		}
	}

	if (bReject)
		QDialog::reject();
}


// end of qjackctlSetupForm.cpp
