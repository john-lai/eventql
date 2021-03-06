/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#ifndef _FNORD_TSDB_STREAMCHUNK_H
#define _FNORD_TSDB_STREAMCHUNK_H
#include <eventql/util/stdtypes.h>
#include <eventql/util/option.h>
#include <eventql/util/UnixTime.h>
#include <eventql/util/protobuf/MessageSchema.h>
#include <eventql/util/http/httpconnectionpool.h>
#include <eventql/db/server_config.h>
#include <eventql/db/table.h>
#include <eventql/db/record_ref.h>
#include <eventql/db/partition_info.pb.h>
#include <eventql/db/partition_snapshot.h>
#include <eventql/db/metadata_transaction.h>
#include <eventql/db/metadata_operations.pb.h>
#include <eventql/io/cstable/cstable_reader.h>

#include "eventql/eventql.h"

namespace eventql {
class Table;
class PartitionReader;
class PartitionReplication;
class PartitionWriter;
class PartitionMap;

using PartitionKey =
    std::tuple<
        String,     // namespace
        String,     // table
        SHA1Hash>;  // partition

struct KeyRange {
  String begin;
  String end;
  SHA1Hash partition_id;
};

class Partition : public RefCounted {
public:

  static RefPtr<Partition> create(
      const String& tsdb_namespace,
      RefPtr<Table> table,
      const SHA1Hash& partition_key,
      const PartitionDiscoveryResponse& discovery_info,
      ServerCfg* cfg);

  static RefPtr<Partition> reopen(
      const String& tsdb_namespace,
      RefPtr<Table> table,
      const SHA1Hash& partition_key,
      ServerCfg* cfg);

  Partition(
      SHA1Hash partition_id,
      ServerCfg* cfg,
      RefPtr<PartitionSnapshot> snap,
      RefPtr<Table> table);

  ~Partition();

  SHA1Hash getPartitionID() const;
  SHA1Hash uuid() const;

  RefPtr<PartitionWriter> getWriter();
  RefPtr<PartitionReader> getReader();
  RefPtr<PartitionSnapshot> getSnapshot();
  RefPtr<Table> getTable();
  PartitionInfo getInfo() const;

  RefPtr<PartitionReplication> getReplicationStrategy(
      http::HTTPConnectionPool* http);

  String getRelativePath() const;
  String getAbsolutePath() const;

  MetadataTransaction getLastMetadataTransaction() const;
  size_t getTotalDiskSize() const;

  bool isSplitting() const;

protected:

  bool upgradeToLSMv2() const;

  SHA1Hash partition_id_;
  ServerCfg* cfg_;
  PartitionSnapshotRef head_;
  RefPtr<Table> table_;
  RefPtr<PartitionWriter> writer_;
  std::mutex writer_lock_;
};

class LazyPartition {
public:

  LazyPartition();
  LazyPartition(RefPtr<Partition> partition);
  ~LazyPartition();

  RefPtr<Partition> getPartition(
      const String& tsdb_namespace,
      RefPtr<Table> table,
      const SHA1Hash& partition_key,
      ServerCfg* cfg,
      PartitionMap* pmap);

  RefPtr<Partition> getPartition();

  bool isLoaded() const;

protected:
  RefPtr<Partition> partition_;
  mutable std::mutex mutex_;
};

}
#endif
