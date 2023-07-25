#./gradlew assembleRelease
./gradlew assembleDebug
adb -s 6c18398f install ./app/build/outputs/apk/debug/app-debug.apk
adb -s 6c18398f shell am start -n com.twobit.gtmobile/com.twobit.gtmobile.MainActivity


