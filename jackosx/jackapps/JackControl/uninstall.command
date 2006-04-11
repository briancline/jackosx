#!/bin/sh

files="/usr/local/bin/jackd \
	/usr/local/bin/jack_metro \
	/usr/local/bin/jack_lsp \
	/usr/local/bin/jack_disconnect \
	/usr/local/bin/jack_connect \
	/usr/local/lib/jack/jack_portaudio.so \
	/usr/local/lib/jack/jack_coreaudio.so \
	/usr/local/lib/libjack.0.0.23.dylib \
	/usr/local/lib/libjack.dylib \
	/usr/local/lib/libjack.0.dylib \
	/usr/local/lib/pkgconfig/jack.pc \
	/usr/local/include/jack \
	/Library/Frameworks/Jack.framework \
	/Library/Frameworks/Panda.framework \
	/Library/Audio/Plug-Ins/HAL/JackRouter.plugin \
	/Library/Audio/Plug-Ins/Components/JACK-insert.component \ 
	/Library/Audio/Plug-Ins/VST/JACK-insert.vst \
	~/Library/Preferences/JackPilot.plist \
	~/Library/Preferences/JAS.jpil \
	/Applications/Jack \
	/Library/Application Support/JackPilot \
	/Library/Receipts/JackOSX.0.71.pkg"
	
clear
echo
echo "This will remove Jack from your system."
echo
echo "Are you sure you want to continue?"
echo "Press CTRL-C or close this window to abort, RETURN to continue."
read
echo
echo "When prompted for a password, enter your admin password."
echo

for f in $files
do
	if [ -e "$f" ]
	then
    		echo "Removing $f..."
		if ! sudo rm -R "$f"
		then
        		echo "ERROR: Unable to remove $f. Please remove manually."
		 fi
    		echo "$f successfully removed."
    		echo
	else
    		echo "$f not found, skipping."
		echo
	fi
done

touch /System/Library/Extensions

echo "Uninstall completed. You should restart your system now."
echo
