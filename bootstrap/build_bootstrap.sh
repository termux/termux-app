rm -f bootstrap-aarch64.zip
rm -f bootstrap-arm.zip
rm -f bootstrap-i686.zip
rm -f bootstrap-x86_64.zip

wget https://termux.net/bootstrap/bootstrap-aarch64.zip
wget https://termux.net/bootstrap/bootstrap-arm.zip
wget https://termux.net/bootstrap/bootstrap-i686.zip
wget https://termux.net/bootstrap/bootstrap-x86_64.zip

unzip bootstrap-aarch64.zip -d bootstrap-aarch64
unzip bootstrap-arm.zip -d bootstrap-arm
unzip bootstrap-i686.zip -d bootstrap-i686
unzip bootstrap-x86_64.zip -d bootstrap-x86_64

cp -r changes/* bootstrap-aarch64
cp -r changes/* bootstrap-arm
cp -r changes/* bootstrap-i686
cp -r changes/* bootstrap-x86_64

cd bootstrap-aarch64
zip -r ../bootstrap-aarch64.zip *

cd ../bootstrap-arm
zip -r ../bootstrap-arm.zip *

cd ../bootstrap-i686
zip -r ../bootstrap-i686.zip *

cd ../bootstrap-x86_64
zip -r ../bootstrap-x86_64.zip *

cd ../

rm -rf bootstrap-aarch64
rm -rf bootstrap-arm
rm -rf bootstrap-i686
rm -rf bootstrap-x86_64

sum=$(sha256sum bootstrap-aarch64.zip | cut -d " " -f1 | sed 's/^0*//')
sed -i 's#downloadBootstrap("aarch64", ".*", version)#downloadBootstrap("aarch64", "'$sum'", version)#' ../app/build.gradle 

sum=$(sha256sum bootstrap-arm.zip | cut -d " " -f1 | sed 's/^0*//')
sed -i 's#downloadBootstrap("arm",     ".*", version)#downloadBootstrap("arm",     "'$sum'", version)#' ../app/build.gradle 

sum=$(sha256sum bootstrap-i686.zip | cut -d " " -f1 | sed 's/^0*//')
sed -i 's#downloadBootstrap("i686",    ".*", version)#downloadBootstrap("i686",    "'$sum'", version)#' ../app/build.gradle 

sum=$(sha256sum bootstrap-x86_64.zip | cut -d " " -f1 | sed 's/^0*//')
sed -i 's#downloadBootstrap("x86_64",  ".*", version)#downloadBootstrap("x86_64",  "'$sum'", version)#' ../app/build.gradle 
