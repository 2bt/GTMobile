#./gradlew assembleRelease
./gradlew assembleDebug
du -h ./app/build/outputs/apk/debug/app-debug.apk
adb install ./app/build/outputs/apk/debug/app-debug.apk
adb shell am start -n com.twobit.gtmobile/com.twobit.gtmobile.MainActivity

