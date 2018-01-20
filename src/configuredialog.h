/***************************************************************************
 *   Copyright (C) 2005 by Danny Kukawka                                   *
 *                         <dkukawka@suse.de>, <danny.kukawka@web.de>      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of version 2 of the GNU General Public License     *
 *   as published by the Free Software Foundation.                         *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/
 
#ifndef CONFIGUREDIALOG_H
#define CONFIGUREDIALOG_H

/*! 
*  \file 	configuredialog.h
*  \brief 	Headerfile for configureDialog.cpp and the class \ref ConfigureDialog.
*/
/*! 
*  \class 	ConfigureDialog
*  \brief 	class for all config dialog related funtionality
*  \author 	Danny Kukawka, <dkukawka@suse.de>, <danny.kukawka@web.de>
*  \date    	2005
*/

// own header
#include "blacklisteditdialog.h"
#include "settings.h"
#include "hardware.h"

// header of the UI
#include "configure_Dialog.h"

class ConfigureDialog: public configure_Dialog {
	
	Q_OBJECT

public:
	//! default constructor
	ConfigureDialog( KConfig *_config, HardwareInfo *_hwinfo, Settings *_settings,
			 QWidget *parent = 0, const char *name = 0);
	//! default destructor
	~ConfigureDialog();
	
private:
	
	//! pointer to instance of the blacklist edit dialog for Autosuspend
	blacklistEditDialog* blacklistEDlgAS;
	//! pointer to instance of the blacklist edit dialog for Autodimm
	blacklistEditDialog* blacklistEDlgAD;

	//! the pointer to the config of kpowersave, get from constructor
	KConfig *kconfig;
	//! pointer to the KPowersave settings, get from constructor
	Settings *settings;
	//! pointer to hardware information and actions
	HardwareInfo *hwinfo;
	//! struct with allowed suspend states
	SuspendStates suspend;

	//! QStringList with the blacklisted processes
	QStringList blacklist;
	//! QStringList with all available supported actions
	QStringList actions;
	//! QStringList with the name of the schemes in the config
	QStringList schemes;

	//! true if the current desktop session is a GNOME session, else if not
	bool gnome_session;
	
	//! true if a value within the Tab 'General Settings' is changed, else if not
	bool general_changed;
	//! true if all is initialised, false if not
	bool initalised;
	//! true if a value within the Tab 'Scheme Settings' is changed, else if not
	bool scheme_changed;
	//! true if the warning message was displayed , else if not!
	bool displayed_WARN_autosuspend;
	//! true if the brightness was changed
	bool brightness_changed;
	
	//! represent the ID of the current selected scheme related to the schemeList
	int currentScheme;

	//! the max numbers of levels supported by the machine
	int brightnessLevels;
	//! the brightness as the configdialog was started
	int brightness_last;
	
	//! to store the changed 'General' setttings 
	void saveGeneralSettings();
	//! to store the (changed) settings of the current scheme
	void saveSchemeSettings();
	//! set all needed widgets in the dialog for a given ID of a scheme

	void setConfigToDialog( int );
	//! set the values from the section general in configfile to dialog
	void setGeneralSettings();
	//! set all needed icons
	void setIcons();
	//! set all needed tooltips
	void setTooltips();
	//! set the gB_inactivity visible/invisible
	void setInactivityBox();
	
	//! set the schemelist to the listbox
	void setSchemeList();
	//! get the list of schemes and fill \ref schemes
	void getSchemeList();
	//! set the current scheme
	void selectScheme (QString _scheme);
	//! to get the real Name of the Scheme
	QString getSchemeRealName( QString );

	//! map a action string from options to a description
	QString mapActionToDescription( QString action );
	//! map a (translated) description of a action back to the name config key
	QString mapDescriptionToAction( QString description );
	//! fill a QComboBox with actions (translated text) and select a value
	void fillActionComboBox(QComboBox *_cb, QStringList _actions, QString _select);
	
signals:

	//! signal to open the Help
	void openHelp();
	//! signal to open the KNotify dialog
	void openKNotify();

private slots:

	//! alled if the user click on 'Apply' Button
	void buttonApply_clicked();
	//! called if the user click on 'Chancel' Button
	void buttonCancel_clicked();
	//! called if the user click on 'OK' Button
	void buttonOk_clicked();
	//! called if the user click on 'Help' Button
	void buttonHelp_clicked();
	
	//! called if the 'Enable scheme specific blacklist' checkbox toggled
	void cB_Blacklist_toggled( bool );
	//! called if the 'Disable screensaver' checkbox toggled
	void cB_disable_Ss_toggled( bool );
	//! called if the 'disablePM' checkbox toggled
	void cB_disablePM_toggled( bool );
	//! called if the 'SpecificPM' checkbox toggled
	void cB_SpecificPM_toggled( bool );
	//! called if the 'specificSettings' checkbox toggled
	void cB_specificSettings_toggled( bool );
	
	//! called if the checkbox cB_autoSuspend toggled
	void cB_autoSuspend_toggled( bool );
	//! called if the current comboB_autoInactivity item is changed
	void cB_autoInactivity_activated( int );
	
	//! called if the checkbox cB_autoDimm toggled
	void cB_autoDimm_toggled( bool );
	//! called if the 'Enable scheme specific blacklist' checkbox toggled
	void cB_BlacklistDimm_toggled( bool );
	//! called if the value of sB_autoDimmTime get changed
	void sB_autoDimmTime_valueChanged( int );

	//! called if a value in section 'general' changed
	void general_valueChanged();
	//! called if the current 'schemes' ListBoxItem changed
	void listBox_schemes_currentChanged();
	//! called if a checkbox in section 'scheme' changed
	void scheme_valueChanged();
	
	//! called if the pB_editBlacklist clicked
	void pB_editBlacklistSuspend_clicked();
	//! to store the changed and from BlacklistEditDlg recieved suspend blacklist to scheme
	void saveSchemeSuspendBlacklist( QStringList );
	//! called if the pB_editBlacklistDimm clicked
	void pB_editBlacklistDimm_clicked();
	//! to store the changed and from BlacklistEditDlg recieved dimm blacklist to scheme
	void saveSchemeDimmBlacklist( QStringList );

	//! called if the checkbox cB_Brightness toggled
	void cB_Brightness_toggled ( bool );
	//! called if the slider for brightness changed
	void brightnessSlider_sliderMoved( int );
	//! called if pB_resetBrightness clicked
	void pB_resetBrightness_clicked();
	//! called if pB_configNotify released
	void pB_configNotify_released();

	//! called if the button for a new scheme get clicked
	void pB_newScheme_clicked();
	//! called if the button for delete a scheme get clicked
	void pB_deleteScheme_clicked();

	//! called if the value of the standby DPMS spinBox changed
	void sB_standby_valueChanged();
	//! called if the value of the suspend DPMS spinBox changed
	void sB_suspend_valueChanged();
	//! called if the value of the powerOff DPMS spinBox changed
	void sB_powerOff_valueChanged();

	//! called if the warning QSpinBoxes for battery level is changed
	void sB_batWarning_valueChanged();
	//! called if the low QSpinBoxes for battery level is changed
	void sB_batLow_valueChanged();
	//! called if the critical QSpinBoxes for battery level is changed
	void sB_batCritical_valueChanged();

	//! called if the QComboBox for the battery warning level action changed
	void cB_batWarning_activated();
	//! called if the QComboBox for the battery low level action changed
	void cB_batLow_activated();
	//! called if the QComboBox for the battery critical level action changed
	void cB_batCritical_activated();

};

#endif

