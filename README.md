# Azure-Boost-Observability

Azure-Boost-Observability is a Windows command-line utility for interacting with NVMe™ drives on Azure Boost-enabled virtual machines. It provides functionality to query performance statistics and identify NVMe™ namespaces.

## Trademark

This project may contain trademarks or logos for projects, products, or services. Authorized use of Microsoft trademarks or logos is subject to and must follow Microsoft�s Trademark & Brand Guidelines. Use of Microsoft trademarks or logos in modified versions of this project must not cause confusion or imply Microsoft sponsorship. Any use of third-party trademarks or logos are subject to those third-party�s policies.

## Features

- **Performance Statistics**: Query performance counters and statistics from NVMe™ drives
- **Device Identification**: Identify NVMe™ namespaces and map them from your guest VM to the Azure Portal
- **Batch Processing**: Query all NVMe™ drives in the system automatically
- **Verbose Output**: Optional detailed logging for troubleshooting

## Usage

```
Azure-Boost-Observability.exe [/json] [/verbose] (/perf|/identify) [<physical-drive-no>|<drive-letter>]
```

### Commands

- `/perf` - Get performance statistics log page
- `/identify` - Return the name, type, size and NGUID of the NVMe™ namespace

### Arguments

- `<physical-drive-no>` - Physical number of drive to be accessed (e.g., `0`, `1`, `2`)
- `<drive-letter>` - Letter of the drive to be accessed (e.g., `C`, `D`)

### Options

- `/verbose` - Enable verbose output for detailed logging, needs to be specified before any other command
- `/json` - Enable json format output, needs to be specified before any other command

## Examples

### Query Performance Statistics

```bash
# Query performance stats for all NVMe™ drives
Azure-Boost-Observability.exe /perf
Azure-Boost-Observability.exe /json /perf

# Query performance counters for disk 0
Azure-Boost-Observability.exe /perf 0
Azure-Boost-Observability.exe /json /perf 0
```

### Identify NVMe™ Devices

```bash
# Get friendly names for all NVMe™ drives
Azure-Boost-Observability.exe /identify
Azure-Boost-Observability.exe /json /identify

# Get friendly name for disk 0
Azure-Boost-Observability.exe /identify 0
Azure-Boost-Observability.exe /json /identify 0
```

## Performance Metrics

The performance statistics are retrieved from the NVMe™ device's log pages and include metrics such as:

- **is degraded**: A boolean as to whether the disk is in a degraded performance state.
- **total read IOPs**: The total read IOPs completed during the disk's lifecycle.
- **total write IOPs**: The total write IOPs completed during the disk's lifecycle.
- **total read bytes**: The total read bytes completed during the disk's lifecycle.
- **total written bytes**: The total written bytes completed during the disk's lifecycle.
- **outstanding IO**: The total IO requests outstanding in the disk.
- **read_latency_max**: The maximum read latency observed during the disk's lifecycle.
- **read_latency_avg**: The average read latency observed between IOPs during the sampling interval.
- **write_latency_max**: The maximum write latency observed during the disk's lifecycle.
- **write_latency_avg**: The average write latency observed between IOPs during the sampling interval.
- **read_dispatch_latency_max**: The maximum read disk latency during the disk's lifecycle.
- **read_dispatch_latency_avg**: The average read disk latency observed in the disk scheduler during the sampling interval.
- **write_dispatch_latency_max**: The maximum write disk latency during the disk's lifecycle.
- **write_dispatch_latency_avg**: The average write disk latency observed in the disk the scheduler during the sampling interval.


## Disk identification

Disk identification data is retrieved from the NVMe™ device using the Identify Namespace command and returns the following fields:

- **Disk**: The Windows disk number identifying the queried device.
- **NGUID**: The 128-bit Namespace Globally Unique Identifier reported by the NVMe™ device, formatted as a GUID. Displays `undefined` if no identifier is provided.
- **Disk Size**: The namespace size in gibibytes (GiB), calculated from the number of logical blocks and the logical block size. One GiB is (1024^3) bytes.
- **Vendor Specific**: The identification string stored in the namespace's vendor-specific data area. Displays `undefined` if the string is empty. The string contains:
  - **type**: The disk role: `unknown`, `data` (data disk), `os` (operating system disk), or `local` (local disk).
  - **name**: The friendly name assigned to the disk.

## Requirements

- Windows operating system
- Administrative privileges may be required for direct NVMe™ device access
- Compatible NVMe™ drives on Azure Boost-enabled virtual machines

## Implementation Details

- Maximum of 32 disks can be queried in batch mode
- Uses Windows NVMe™ pass-through interface
- Supports both physical drive numbers and drive letters for device targeting