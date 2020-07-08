################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Ensc351Part2.cpp \
../Medium.cpp \
../PeerX.cpp \
../ReceiverX.cpp \
../SenderX.cpp \
../TestFile.cpp \
../main.cpp \
../myIO.cpp \
../myIO_conflict-20191018-154049.cpp 

OBJS += \
./Ensc351Part2.o \
./Medium.o \
./PeerX.o \
./ReceiverX.o \
./SenderX.o \
./TestFile.o \
./main.o \
./myIO.o \
./myIO_conflict-20191018-154049.o 

CPP_DEPS += \
./Ensc351Part2.d \
./Medium.d \
./PeerX.d \
./ReceiverX.d \
./SenderX.d \
./TestFile.d \
./main.d \
./myIO.d \
./myIO_conflict-20191018-154049.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++1y -I"/mnt/hgfs/VMsf/git/ensc351lib/ /Ensc351" -I"/mnt/hgfs/VMsf/workspace-cpp-OxygenSeptember/Ensc351Part2CodedBySmartState/src" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


