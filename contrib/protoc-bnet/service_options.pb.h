// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: service_options.proto

#ifndef PROTOBUF_service_5foptions_2eproto__INCLUDED
#define PROTOBUF_service_5foptions_2eproto__INCLUDED

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 2006000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 2006001 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/unknown_field_set.h>
#include "google/protobuf/descriptor.pb.h"
// @@protoc_insertion_point(includes)

namespace Battlenet {

// Internal implementation detail -- do not call these.
void  protobuf_AddDesc_service_5foptions_2eproto();
void protobuf_AssignDesc_service_5foptions_2eproto();
void protobuf_ShutdownFile_service_5foptions_2eproto();

class BGSServiceOptions;
class SDKServiceOptions;

// ===================================================================

class BGSServiceOptions : public ::google::protobuf::Message {
 public:
  BGSServiceOptions();
  virtual ~BGSServiceOptions();

  BGSServiceOptions(const BGSServiceOptions& from);

  inline BGSServiceOptions& operator=(const BGSServiceOptions& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }

  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const BGSServiceOptions& default_instance();

  void Swap(BGSServiceOptions* other);

  // implements Message ----------------------------------------------

  BGSServiceOptions* New() const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:
  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // optional string descriptor_name = 1;
  inline bool has_descriptor_name() const;
  inline void clear_descriptor_name();
  static const int kDescriptorNameFieldNumber = 1;
  inline const ::std::string& descriptor_name() const;
  inline void set_descriptor_name(const ::std::string& value);
  inline void set_descriptor_name(const char* value);
  inline void set_descriptor_name(const char* value, size_t size);
  inline ::std::string* mutable_descriptor_name();
  inline ::std::string* release_descriptor_name();
  inline void set_allocated_descriptor_name(::std::string* descriptor_name);

  // optional uint32 version = 4;
  inline bool has_version() const;
  inline void clear_version();
  static const int kVersionFieldNumber = 4;
  inline ::google::protobuf::uint32 version() const;
  inline void set_version(::google::protobuf::uint32 value);

  // optional string shard_name = 5;
  inline bool has_shard_name() const;
  inline void clear_shard_name();
  static const int kShardNameFieldNumber = 5;
  inline const ::std::string& shard_name() const;
  inline void set_shard_name(const ::std::string& value);
  inline void set_shard_name(const char* value);
  inline void set_shard_name(const char* value, size_t size);
  inline ::std::string* mutable_shard_name();
  inline ::std::string* release_shard_name();
  inline void set_allocated_shard_name(::std::string* shard_name);

  // @@protoc_insertion_point(class_scope:Battlenet.BGSServiceOptions)
 private:
  inline void set_has_descriptor_name();
  inline void clear_has_descriptor_name();
  inline void set_has_version();
  inline void clear_has_version();
  inline void set_has_shard_name();
  inline void clear_has_shard_name();

  ::google::protobuf::UnknownFieldSet _unknown_fields_;

  ::google::protobuf::uint32 _has_bits_[1];
  mutable int _cached_size_;
  ::std::string* descriptor_name_;
  ::std::string* shard_name_;
  ::google::protobuf::uint32 version_;
  friend void  protobuf_AddDesc_service_5foptions_2eproto();
  friend void protobuf_AssignDesc_service_5foptions_2eproto();
  friend void protobuf_ShutdownFile_service_5foptions_2eproto();

  void InitAsDefaultInstance();
  static BGSServiceOptions* default_instance_;
};
// -------------------------------------------------------------------

class SDKServiceOptions : public ::google::protobuf::Message {
 public:
  SDKServiceOptions();
  virtual ~SDKServiceOptions();

  SDKServiceOptions(const SDKServiceOptions& from);

  inline SDKServiceOptions& operator=(const SDKServiceOptions& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }

  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const SDKServiceOptions& default_instance();

  void Swap(SDKServiceOptions* other);

  // implements Message ----------------------------------------------

  SDKServiceOptions* New() const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:
  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // optional bool inbound = 1;
  inline bool has_inbound() const;
  inline void clear_inbound();
  static const int kInboundFieldNumber = 1;
  inline bool inbound() const;
  inline void set_inbound(bool value);

  // optional bool outbound = 2;
  inline bool has_outbound() const;
  inline void clear_outbound();
  static const int kOutboundFieldNumber = 2;
  inline bool outbound() const;
  inline void set_outbound(bool value);

  // optional bool use_client_id = 3;
  inline bool has_use_client_id() const;
  inline void clear_use_client_id();
  static const int kUseClientIdFieldNumber = 3;
  inline bool use_client_id() const;
  inline void set_use_client_id(bool value);

  // @@protoc_insertion_point(class_scope:Battlenet.SDKServiceOptions)
 private:
  inline void set_has_inbound();
  inline void clear_has_inbound();
  inline void set_has_outbound();
  inline void clear_has_outbound();
  inline void set_has_use_client_id();
  inline void clear_has_use_client_id();

  ::google::protobuf::UnknownFieldSet _unknown_fields_;

  ::google::protobuf::uint32 _has_bits_[1];
  mutable int _cached_size_;
  bool inbound_;
  bool outbound_;
  bool use_client_id_;
  friend void  protobuf_AddDesc_service_5foptions_2eproto();
  friend void protobuf_AssignDesc_service_5foptions_2eproto();
  friend void protobuf_ShutdownFile_service_5foptions_2eproto();

  void InitAsDefaultInstance();
  static SDKServiceOptions* default_instance_;
};
// ===================================================================

static const int kServiceOptionsFieldNumber = 90000;
extern ::google::protobuf::internal::ExtensionIdentifier< ::google::protobuf::ServiceOptions,
    ::google::protobuf::internal::MessageTypeTraits< ::Battlenet::BGSServiceOptions >, 11, false >
  service_options;
static const int kSdkServiceOptionsFieldNumber = 90001;
extern ::google::protobuf::internal::ExtensionIdentifier< ::google::protobuf::ServiceOptions,
    ::google::protobuf::internal::MessageTypeTraits< ::Battlenet::SDKServiceOptions >, 11, false >
  sdk_service_options;

// ===================================================================

// BGSServiceOptions

// optional string descriptor_name = 1;
inline bool BGSServiceOptions::has_descriptor_name() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void BGSServiceOptions::set_has_descriptor_name() {
  _has_bits_[0] |= 0x00000001u;
}
inline void BGSServiceOptions::clear_has_descriptor_name() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void BGSServiceOptions::clear_descriptor_name() {
  if (descriptor_name_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    descriptor_name_->clear();
  }
  clear_has_descriptor_name();
}
inline const ::std::string& BGSServiceOptions::descriptor_name() const {
  // @@protoc_insertion_point(field_get:Battlenet.BGSServiceOptions.descriptor_name)
  return *descriptor_name_;
}
inline void BGSServiceOptions::set_descriptor_name(const ::std::string& value) {
  set_has_descriptor_name();
  if (descriptor_name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    descriptor_name_ = new ::std::string;
  }
  descriptor_name_->assign(value);
  // @@protoc_insertion_point(field_set:Battlenet.BGSServiceOptions.descriptor_name)
}
inline void BGSServiceOptions::set_descriptor_name(const char* value) {
  set_has_descriptor_name();
  if (descriptor_name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    descriptor_name_ = new ::std::string;
  }
  descriptor_name_->assign(value);
  // @@protoc_insertion_point(field_set_char:Battlenet.BGSServiceOptions.descriptor_name)
}
inline void BGSServiceOptions::set_descriptor_name(const char* value, size_t size) {
  set_has_descriptor_name();
  if (descriptor_name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    descriptor_name_ = new ::std::string;
  }
  descriptor_name_->assign(reinterpret_cast<const char*>(value), size);
  // @@protoc_insertion_point(field_set_pointer:Battlenet.BGSServiceOptions.descriptor_name)
}
inline ::std::string* BGSServiceOptions::mutable_descriptor_name() {
  set_has_descriptor_name();
  if (descriptor_name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    descriptor_name_ = new ::std::string;
  }
  // @@protoc_insertion_point(field_mutable:Battlenet.BGSServiceOptions.descriptor_name)
  return descriptor_name_;
}
inline ::std::string* BGSServiceOptions::release_descriptor_name() {
  clear_has_descriptor_name();
  if (descriptor_name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    return NULL;
  } else {
    ::std::string* temp = descriptor_name_;
    descriptor_name_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
    return temp;
  }
}
inline void BGSServiceOptions::set_allocated_descriptor_name(::std::string* descriptor_name) {
  if (descriptor_name_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete descriptor_name_;
  }
  if (descriptor_name) {
    set_has_descriptor_name();
    descriptor_name_ = descriptor_name;
  } else {
    clear_has_descriptor_name();
    descriptor_name_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  }
  // @@protoc_insertion_point(field_set_allocated:Battlenet.BGSServiceOptions.descriptor_name)
}

// optional uint32 version = 4;
inline bool BGSServiceOptions::has_version() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void BGSServiceOptions::set_has_version() {
  _has_bits_[0] |= 0x00000002u;
}
inline void BGSServiceOptions::clear_has_version() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void BGSServiceOptions::clear_version() {
  version_ = 0u;
  clear_has_version();
}
inline ::google::protobuf::uint32 BGSServiceOptions::version() const {
  // @@protoc_insertion_point(field_get:Battlenet.BGSServiceOptions.version)
  return version_;
}
inline void BGSServiceOptions::set_version(::google::protobuf::uint32 value) {
  set_has_version();
  version_ = value;
  // @@protoc_insertion_point(field_set:Battlenet.BGSServiceOptions.version)
}

// optional string shard_name = 5;
inline bool BGSServiceOptions::has_shard_name() const {
  return (_has_bits_[0] & 0x00000004u) != 0;
}
inline void BGSServiceOptions::set_has_shard_name() {
  _has_bits_[0] |= 0x00000004u;
}
inline void BGSServiceOptions::clear_has_shard_name() {
  _has_bits_[0] &= ~0x00000004u;
}
inline void BGSServiceOptions::clear_shard_name() {
  if (shard_name_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    shard_name_->clear();
  }
  clear_has_shard_name();
}
inline const ::std::string& BGSServiceOptions::shard_name() const {
  // @@protoc_insertion_point(field_get:Battlenet.BGSServiceOptions.shard_name)
  return *shard_name_;
}
inline void BGSServiceOptions::set_shard_name(const ::std::string& value) {
  set_has_shard_name();
  if (shard_name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    shard_name_ = new ::std::string;
  }
  shard_name_->assign(value);
  // @@protoc_insertion_point(field_set:Battlenet.BGSServiceOptions.shard_name)
}
inline void BGSServiceOptions::set_shard_name(const char* value) {
  set_has_shard_name();
  if (shard_name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    shard_name_ = new ::std::string;
  }
  shard_name_->assign(value);
  // @@protoc_insertion_point(field_set_char:Battlenet.BGSServiceOptions.shard_name)
}
inline void BGSServiceOptions::set_shard_name(const char* value, size_t size) {
  set_has_shard_name();
  if (shard_name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    shard_name_ = new ::std::string;
  }
  shard_name_->assign(reinterpret_cast<const char*>(value), size);
  // @@protoc_insertion_point(field_set_pointer:Battlenet.BGSServiceOptions.shard_name)
}
inline ::std::string* BGSServiceOptions::mutable_shard_name() {
  set_has_shard_name();
  if (shard_name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    shard_name_ = new ::std::string;
  }
  // @@protoc_insertion_point(field_mutable:Battlenet.BGSServiceOptions.shard_name)
  return shard_name_;
}
inline ::std::string* BGSServiceOptions::release_shard_name() {
  clear_has_shard_name();
  if (shard_name_ == &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    return NULL;
  } else {
    ::std::string* temp = shard_name_;
    shard_name_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
    return temp;
  }
}
inline void BGSServiceOptions::set_allocated_shard_name(::std::string* shard_name) {
  if (shard_name_ != &::google::protobuf::internal::GetEmptyStringAlreadyInited()) {
    delete shard_name_;
  }
  if (shard_name) {
    set_has_shard_name();
    shard_name_ = shard_name;
  } else {
    clear_has_shard_name();
    shard_name_ = const_cast< ::std::string*>(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  }
  // @@protoc_insertion_point(field_set_allocated:Battlenet.BGSServiceOptions.shard_name)
}

// -------------------------------------------------------------------

// SDKServiceOptions

// optional bool inbound = 1;
inline bool SDKServiceOptions::has_inbound() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void SDKServiceOptions::set_has_inbound() {
  _has_bits_[0] |= 0x00000001u;
}
inline void SDKServiceOptions::clear_has_inbound() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void SDKServiceOptions::clear_inbound() {
  inbound_ = false;
  clear_has_inbound();
}
inline bool SDKServiceOptions::inbound() const {
  // @@protoc_insertion_point(field_get:Battlenet.SDKServiceOptions.inbound)
  return inbound_;
}
inline void SDKServiceOptions::set_inbound(bool value) {
  set_has_inbound();
  inbound_ = value;
  // @@protoc_insertion_point(field_set:Battlenet.SDKServiceOptions.inbound)
}

// optional bool outbound = 2;
inline bool SDKServiceOptions::has_outbound() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void SDKServiceOptions::set_has_outbound() {
  _has_bits_[0] |= 0x00000002u;
}
inline void SDKServiceOptions::clear_has_outbound() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void SDKServiceOptions::clear_outbound() {
  outbound_ = false;
  clear_has_outbound();
}
inline bool SDKServiceOptions::outbound() const {
  // @@protoc_insertion_point(field_get:Battlenet.SDKServiceOptions.outbound)
  return outbound_;
}
inline void SDKServiceOptions::set_outbound(bool value) {
  set_has_outbound();
  outbound_ = value;
  // @@protoc_insertion_point(field_set:Battlenet.SDKServiceOptions.outbound)
}

// optional bool use_client_id = 3;
inline bool SDKServiceOptions::has_use_client_id() const {
  return (_has_bits_[0] & 0x00000004u) != 0;
}
inline void SDKServiceOptions::set_has_use_client_id() {
  _has_bits_[0] |= 0x00000004u;
}
inline void SDKServiceOptions::clear_has_use_client_id() {
  _has_bits_[0] &= ~0x00000004u;
}
inline void SDKServiceOptions::clear_use_client_id() {
  use_client_id_ = false;
  clear_has_use_client_id();
}
inline bool SDKServiceOptions::use_client_id() const {
  // @@protoc_insertion_point(field_get:Battlenet.SDKServiceOptions.use_client_id)
  return use_client_id_;
}
inline void SDKServiceOptions::set_use_client_id(bool value) {
  set_has_use_client_id();
  use_client_id_ = value;
  // @@protoc_insertion_point(field_set:Battlenet.SDKServiceOptions.use_client_id)
}


// @@protoc_insertion_point(namespace_scope)

}  // namespace Battlenet

#ifndef SWIG
namespace google {
namespace protobuf {


}  // namespace google
}  // namespace protobuf
#endif  // SWIG

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_service_5foptions_2eproto__INCLUDED