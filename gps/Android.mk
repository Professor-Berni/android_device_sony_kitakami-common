#
# Copyright (C) 2015 The CyanogenMod Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

LOCAL_PATH := $(call my-dir)

# Only compile the gps.conf/izat.conf/sap.conf/flp.conf prebuilts from this
# subtree. Skip utils/, core/, loc_api/: the stock vendor libloc_api_v02.so
# ABI-mismatches CAF source built with our toolchain, so the matching stock
# blobs are shipped to keep the ABI consistent end-to-end.
include $(LOCAL_PATH)/etc/Android.mk
