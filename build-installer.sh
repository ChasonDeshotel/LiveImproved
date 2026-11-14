NSString* domain = @"com.ableton.live";
NSDictionary* prefs = [[NSUserDefaults standardUserDefaults] persistentDomainForName:domain];
NSString* scriptsPath = prefs[@"RemoteScriptsSearchPath"];
NSArray* fallbacks = @[
    [@"~/Music/Ableton/User Library/Remote Scripts" stringByExpandingTildeInPath],
    [@"~/Music/Ableton/Remote Scripts" stringByExpandingTildeInPath],
];


#
#
#
#
# package the app bundle
pkgbuild --install-location /Applications \
         --component LiveImproved.app \
         LiveImproved.pkg

# package the MIDI scripts
pkgbuild --install-location "/Users/Shared/Ableton/Remote Scripts" \
         --component LiveImprovedRemote \
         MIDIScripts.pkg

# combine into a distributable installer with a nice UI
productbuild --distribution Distribution.xml \
             --package-path . \
             LiveImprovedInstaller.pkg
