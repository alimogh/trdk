//
//    Created: 2018/05/19 15:17
//     Author: Eugene V. Palchukovsky
//     E-mail: eugene@palchukovsky.com
// ------------------------------------------
//    Project: Trading Robot Development Kit
//        URL: http://robotdk.com
//  Copyright: Eugene V. Palchukovsky
//

function Component() {}

Component.prototype.createOperations = function() {
	component.createOperations();

	if (installer.value("os") === "win") {
		component.addOperation("CreateShortcut",
			"@TargetDir@/%{NameFile}%{Suffix}.exe",
			"@StartMenuDir@/" + installer.value("Name") + "%{SuffixHumanReadable}.lnk",
			"workingDirectory=@TargetDir@");
		component.addOperation("CreateShortcut", "@TargetDir@/Maintenance.exe",
			"@StartMenuDir@/" + installer.value("Name") + " Maintenance.lnk",
			"workingDirectory=@TargetDir@");
		component.addOperation("CreateShortcut", installer.value("ProductUrl"),
			"@StartMenuDir@/" + installer.value("Name") + " Web Site.url");
	}
}
