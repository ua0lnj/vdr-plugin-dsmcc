/*
 * dsmcc.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id: dsmcc.c,v 1.1.1.1 2003/10/29 14:14:42 rdp123 Exp $
 */

#include <vdr/plugin.h>
#include "dsmcc-monitor.h"
#include "dsmcc-decoder.h"
#include "dsmcc-siinfo.h"
// #include <mpatrol.h>

static const char *VERSION        = "0.4.0";
static const char *DESCRIPTION    = "Receive DSM-CC data and decode";
//static const char *MAINMENUENTRY  = "Dsmcc";

class cPluginDsmcc : public cPlugin {
private:
  // Add any member variables or functions you may need here.
  cDsmccMonitor *cDsmccStatus;
public:
  cPluginDsmcc(void);
  virtual ~cPluginDsmcc();
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return DESCRIPTION; }
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Start(void);
  virtual void Housekeeping(void);
//  virtual const char *MainMenuEntry(void) { return MAINMENUENTRY; }
//  virtual const char *MainMenuEntry(void) { return NULL; }
//  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);
};

cPluginDsmcc::cPluginDsmcc(void)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!

  cDsmccStatus = NULL;
  ResetMetadataPids();
}

cPluginDsmcc::~cPluginDsmcc()
{
  // Clean up after yourself!
  delete cDsmccStatus;

  // Kill Metadata pids
  KillMetadataPids();
}

const char *cPluginDsmcc::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return " -d debug dsmcc\n";
}

bool cPluginDsmcc::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  return true;
}

bool cPluginDsmcc::Start(void)
{
  // Start any background activities the plugin shall perform.
  cDsmccStatus = new cDsmccMonitor;

//  cDsmccStatus->ScanChannels(5); // number of channels you want to scan

  return true;
}

void cPluginDsmcc::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}
/*
cOsdObject *cPluginDsmcc::MainMenuAction(void)
{
//  cDsmccStatus->ScanChannels(20);
  // Perform the action when selected from the main VDR menu.
  return NULL;
}
*/
cMenuSetupPage *cPluginDsmcc::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return NULL;
}

bool cPluginDsmcc::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
  return false;
}

VDRPLUGINCREATOR(cPluginDsmcc); // Don't touch this!
