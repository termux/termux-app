from eclipse-temurin:11.0.21_9-jdk

ARG ANDROID_SDK_ROOT="/usr/lib/android-sdk"
ENV ANDROID_SDK_ROOT=$ANDROID_SDK_ROOT

RUN apt-get update
RUN apt-get install -y gradle vim android-sdk sdkmanager make file nodejs

WORKDIR $ANDROID_SDK_ROOT
RUN sdkmanager "ndk;22.1.7171670" "ndk;21.4.7075529" "build-tools;29.0.3" "build-tools;30.0.2"
RUN yes | sdkmanager --licenses

WORKDIR /termux-app
COPY . /termux-app

CMD ["bash"]
