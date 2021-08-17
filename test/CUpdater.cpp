//  $Id: CUpdater.cpp,v 1.11 2021/08/17 05:40:05 cvsuser Exp $
//
//  AutoUpdater -- dialog test application.
//
//  This file is part of libautoupdater (https://github.com/adamyg/libappupdater)
//
//  Copyright (c) 2012 - 2021 Adam Young
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in all
//  copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//  SOFTWARE.
//

#include "Stdafx.h"

#include "CUpdater.h"
#include "../src/AutoUpdater.h"

// CUpdaterApp
BEGIN_MESSAGE_MAP(CUpdaterApp, CWinApp)
    ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

// One and only CUpdaterApp object
CUpdaterApp updater_application;

// construction
CUpdaterApp::CUpdaterApp()
{
}

// initialization
BOOL
CUpdaterApp::InitInstance()
{
    return TRUE;
}

// execution
int
CUpdaterApp::Run()
{
    AutoUpdater au;

    au.EnableDialog();
    au.AppName("AutoUpdate Test Framework");
    au.AppVersion("0.0.1");

  //au.HostURL("file:///./example/grief.manifest");
    au.HostURL("https://sourceforge.net/projects/grief/files/grief.manifest/download");
  //au.HostURL("https://master.dl.sourceforge.net/project/grief/grief.manifest?viasf=1");

    au.Execute(AutoUpdater::ExecuteReinstall, true);

    return TRUE;
}
