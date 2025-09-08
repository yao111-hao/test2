// *************************************************************************
//
// Copyright 2020 Xilinx, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// *************************************************************************
// System address map (through PCI-e BAR2 4MB)
// system_config_address_map 模块是一个手写的、高性能的1对N的AXI-Lite地址解码器和路由器。
// --------------------------------------------------
//   BaseAddr  |  HighAddr |  Module
// --------------------------------------------------
//    0x00000  |  0x00FFF  |  System configuration
// --------------------------------------------------
//    0x01000  |  0x05FFF  |  QDMA subsystem
// --------------------------------------------------
//    0x08000  |  0x0AFFF  |  CMAC subsystem #0
// --------------------------------------------------
//    0x0B000  |  0x0BFFF  |  Packet adapter #0
// --------------------------------------------------
//    0x0C000  |  0x0EFFF  |  CMAC subsystem #1
// --------------------------------------------------
//    0x0F000  |  0x0FFFF  |  Packet adapter #1
// --------------------------------------------------
//    0x10000  |  0x11FFF  |  Sysmon block
// --------------------------------------------------
//    0x14000  |  0x16FFF  |  QDMA AXI Bridge CSR
// --------------------------------------------------
//    0x40000  |  0x6FFFF  |  RDMA subsystem
// --------------------------------------------------
//   0x100000  |  0x1FFFFF |  Box0 @ 250MHz
// --------------------------------------------------
//   0x200000  |  0x2FFFFF |  Box1 @ 322MHz
// --------------------------------------------------

`include "open_nic_shell_macros.vh"
`timescale 1ns/1ps
module system_config_address_map #(
  // 参数: CMAC (以太网MAC) 端口的数量，使得模块可配置
  parameter int NUM_CMAC_PORT = 1
) (
   // --- 单一的 AXI4-Lite 从接口 (Slave Port) ---
  // 这是整个地址映射模块的统一入口, 通常连接到像PCIe控制器这样的上游主设备
  input                         s_axil_awvalid,
  input                  [31:0] s_axil_awaddr,
  output                        s_axil_awready,
  input                         s_axil_wvalid,
  input                  [31:0] s_axil_wdata,
  output                        s_axil_wready,
  output                        s_axil_bvalid,
  output                  [1:0] s_axil_bresp,
  input                         s_axil_bready,
  input                         s_axil_arvalid,
  input                  [31:0] s_axil_araddr,
  output                        s_axil_arready,
  output                        s_axil_rvalid,
  output                 [31:0] s_axil_rdata,
  output                  [1:0] s_axil_rresp,
  input                         s_axil_rready,

   // 1 (scfg模块的完整AXI-Lite接口)
  output                        m_axil_scfg_awvalid,
  output                 [31:0] m_axil_scfg_awaddr,
  input                         m_axil_scfg_awready,
  output                        m_axil_scfg_wvalid,
  output                 [31:0] m_axil_scfg_wdata,
  input                         m_axil_scfg_wready,
  input                         m_axil_scfg_bvalid,
  input                   [1:0] m_axil_scfg_bresp,
  output                        m_axil_scfg_bready,
  output                        m_axil_scfg_arvalid,
  output                 [31:0] m_axil_scfg_araddr,
  input                         m_axil_scfg_arready,
  input                         m_axil_scfg_rvalid,
  input                  [31:0] m_axil_scfg_rdata,
  input                   [1:0] m_axil_scfg_rresp,
  output                        m_axil_scfg_rready,

  // 2 -> 连接到 QDMA 模块
  output                        m_axil_qdma_awvalid,
  output                 [31:0] m_axil_qdma_awaddr,
  input                         m_axil_qdma_awready,
  output                        m_axil_qdma_wvalid,
  output                 [31:0] m_axil_qdma_wdata,
  input                         m_axil_qdma_wready,
  input                         m_axil_qdma_bvalid,
  input                   [1:0] m_axil_qdma_bresp,
  output                        m_axil_qdma_bready,
  output                        m_axil_qdma_arvalid,
  output                 [31:0] m_axil_qdma_araddr,
  input                         m_axil_qdma_arready,
  input                         m_axil_qdma_rvalid,
  input                  [31:0] m_axil_qdma_rdata,
  input                   [1:0] m_axil_qdma_rresp,
  output                        m_axil_qdma_rready,

  // 3 -> 连接到 QDMA CSR (控制状态寄存器) 模块
  output                        m_axil_qdma_csr_awvalid,
  output                 [31:0] m_axil_qdma_csr_awaddr,
  input                         m_axil_qdma_csr_awready,
  output                        m_axil_qdma_csr_wvalid,
  output                 [31:0] m_axil_qdma_csr_wdata,
  input                         m_axil_qdma_csr_wready,
  input                         m_axil_qdma_csr_bvalid,
  input                   [1:0] m_axil_qdma_csr_bresp,
  output                        m_axil_qdma_csr_bready,
  output                        m_axil_qdma_csr_arvalid,
  output                 [31:0] m_axil_qdma_csr_araddr,
  input                         m_axil_qdma_csr_arready,
  input                         m_axil_qdma_csr_rvalid,
  input                  [31:0] m_axil_qdma_csr_rdata,
  input                   [1:0] m_axil_qdma_csr_rresp,
  output                        m_axil_qdma_csr_rready,

  // 4 -> 连接到 Packet Adapter 模块 (打包/解包适配器)
  // 注意: 这是一个向量端口, 位宽由 NUM_CMAC_PORT 决定, 可支持多个CMAC
  output    [NUM_CMAC_PORT-1:0] m_axil_adap_awvalid,
  output [32*NUM_CMAC_PORT-1:0] m_axil_adap_awaddr,
  input     [NUM_CMAC_PORT-1:0] m_axil_adap_awready,
  output    [NUM_CMAC_PORT-1:0] m_axil_adap_wvalid,
  output [32*NUM_CMAC_PORT-1:0] m_axil_adap_wdata,
  input     [NUM_CMAC_PORT-1:0] m_axil_adap_wready,
  input     [NUM_CMAC_PORT-1:0] m_axil_adap_bvalid,
  input   [2*NUM_CMAC_PORT-1:0] m_axil_adap_bresp,
  output    [NUM_CMAC_PORT-1:0] m_axil_adap_bready,
  output    [NUM_CMAC_PORT-1:0] m_axil_adap_arvalid,
  output [32*NUM_CMAC_PORT-1:0] m_axil_adap_araddr,
  input     [NUM_CMAC_PORT-1:0] m_axil_adap_arready,
  input     [NUM_CMAC_PORT-1:0] m_axil_adap_rvalid,
  input  [32*NUM_CMAC_PORT-1:0] m_axil_adap_rdata,
  input   [2*NUM_CMAC_PORT-1:0] m_axil_adap_rresp,
  output    [NUM_CMAC_PORT-1:0] m_axil_adap_rready,

  // 5 -> 连接到 CMAC 模块 (以太网MAC)
  output    [NUM_CMAC_PORT-1:0] m_axil_cmac_awvalid,
  output [32*NUM_CMAC_PORT-1:0] m_axil_cmac_awaddr,
  input     [NUM_CMAC_PORT-1:0] m_axil_cmac_awready,
  output    [NUM_CMAC_PORT-1:0] m_axil_cmac_wvalid,
  output [32*NUM_CMAC_PORT-1:0] m_axil_cmac_wdata,
  input     [NUM_CMAC_PORT-1:0] m_axil_cmac_wready,
  input     [NUM_CMAC_PORT-1:0] m_axil_cmac_bvalid,
  input   [2*NUM_CMAC_PORT-1:0] m_axil_cmac_bresp,
  output    [NUM_CMAC_PORT-1:0] m_axil_cmac_bready,
  output    [NUM_CMAC_PORT-1:0] m_axil_cmac_arvalid,
  output [32*NUM_CMAC_PORT-1:0] m_axil_cmac_araddr,
  input     [NUM_CMAC_PORT-1:0] m_axil_cmac_arready,
  input     [NUM_CMAC_PORT-1:0] m_axil_cmac_rvalid,
  input  [32*NUM_CMAC_PORT-1:0] m_axil_cmac_rdata,
  input   [2*NUM_CMAC_PORT-1:0] m_axil_cmac_rresp,
  output    [NUM_CMAC_PORT-1:0] m_axil_cmac_rready,

   // 6 -> 连接到 RDMA 模块
  output                        m_axil_rdma_awvalid,
  output                 [31:0] m_axil_rdma_awaddr,
  input                         m_axil_rdma_awready,
  output                        m_axil_rdma_wvalid,
  output                 [31:0] m_axil_rdma_wdata,
  input                         m_axil_rdma_wready,
  input                         m_axil_rdma_bvalid,
  input                   [1:0] m_axil_rdma_bresp,
  output                        m_axil_rdma_bready,
  output                        m_axil_rdma_arvalid,
  output                 [31:0] m_axil_rdma_araddr,
  input                         m_axil_rdma_arready,
  input                         m_axil_rdma_rvalid,
  input                  [31:0] m_axil_rdma_rdata,
  input                   [1:0] m_axil_rdma_rresp,
  output                        m_axil_rdma_rready,

  // 7 -> 连接到 Box0 模块 (自定义逻辑)
  output                        m_axil_box0_awvalid,
  output                 [31:0] m_axil_box0_awaddr,
  input                         m_axil_box0_awready,
  output                        m_axil_box0_wvalid,
  output                 [31:0] m_axil_box0_wdata,
  input                         m_axil_box0_wready,
  input                         m_axil_box0_bvalid,
  input                   [1:0] m_axil_box0_bresp,
  output                        m_axil_box0_bready,
  output                        m_axil_box0_arvalid,
  output                 [31:0] m_axil_box0_araddr,
  input                         m_axil_box0_arready,
  input                         m_axil_box0_rvalid,
  input                  [31:0] m_axil_box0_rdata,
  input                   [1:0] m_axil_box0_rresp,
  output                        m_axil_box0_rready,

  // 8 -> 连接到 Box1 模块 (自定义逻辑)
  output                        m_axil_box1_awvalid,
  output                 [31:0] m_axil_box1_awaddr,
  input                         m_axil_box1_awready,
  output                        m_axil_box1_wvalid,
  output                 [31:0] m_axil_box1_wdata,
  input                         m_axil_box1_wready,
  input                         m_axil_box1_bvalid,
  input                   [1:0] m_axil_box1_bresp,
  output                        m_axil_box1_bready,
  output                        m_axil_box1_arvalid,
  output                 [31:0] m_axil_box1_araddr,
  input                         m_axil_box1_arready,
  input                         m_axil_box1_rvalid,
  input                  [31:0] m_axil_box1_rdata,
  input                   [1:0] m_axil_box1_rresp,
  output                        m_axil_box1_rready,

  // 9 -> 连接到 Sysmon 模块 (系统监控)
  output                        m_axil_smon_awvalid,
  output                 [31:0] m_axil_smon_awaddr,
  input                         m_axil_smon_awready,
  output                        m_axil_smon_wvalid,
  output                 [31:0] m_axil_smon_wdata,
  input                         m_axil_smon_wready,
  input                         m_axil_smon_bvalid,
  input                   [1:0] m_axil_smon_bresp,
  output                        m_axil_smon_bready,
  output                        m_axil_smon_arvalid,
  output                 [31:0] m_axil_smon_araddr,
  input                         m_axil_smon_arready,
  input                         m_axil_smon_rvalid,
  input                  [31:0] m_axil_smon_rdata,
  input                   [1:0] m_axil_smon_rresp,
  output                        m_axil_smon_rready,

  // --- 全局时钟与复位 ---
  input                         aclk,
  input                         aresetn
);

   // 定义系统中的从设备数量和各自的索引, 便于管理，共11个
  localparam C_NUM_SLAVES  = 11;

  localparam C_SCFG_INDEX  = 0;
  localparam C_QDMA_INDEX  = 1;
  localparam C_CMAC0_INDEX = 2;
  localparam C_ADAP0_INDEX = 3;
  localparam C_CMAC1_INDEX = 4;
  localparam C_ADAP1_INDEX = 5;
  localparam C_SMON_INDEX  = 6;
  localparam C_QCSR_INDEX  = 7;
  localparam C_RDMA_INDEX  = 8;
  localparam C_BOX1_INDEX  = 9;
  localparam C_BOX0_INDEX  = 10;

   // 定义每个从设备在系统地址空间中的基地址，
  localparam C_SCFG_BASE_ADDR  = 32'h0;
  localparam C_QDMA_BASE_ADDR  = 32'h01000;
  localparam C_CMAC0_BASE_ADDR = 32'h08000;
  localparam C_ADAP0_BASE_ADDR = 32'h0B000;
  localparam C_CMAC1_BASE_ADDR = 32'h0C000;
  localparam C_ADAP1_BASE_ADDR = 32'h0F000;
  localparam C_SMON_BASE_ADDR  = 32'h10000;  // 14 bits
  localparam C_QCSR_BASE_ADDR  = 32'h14000;  // 14 bits
  localparam C_RDMA_BASE_ADDR  = 32'h40000;  // 18 bits
  localparam C_BOX0_BASE_ADDR  = 32'h100000; // 20 bits
  localparam C_BOX1_BASE_ADDR  = 32'h200000; // 20 bits

  // --- 内部信号线定义 ---
  // 定义用于地址转换后的本地地址信号，地址线是32位
   // ... (为每个从设备的读写地址定义独立的wire)
  wire                [31:0] axil_scfg_awaddr;
  wire                [31:0] axil_scfg_araddr;
  wire                [31:0] axil_qdma_awaddr;
  wire                [31:0] axil_qdma_araddr;
  wire                [31:0] axil_cmac0_awaddr;
  wire                [31:0] axil_cmac0_araddr;
  wire                [31:0] axil_adap0_awaddr;
  wire                [31:0] axil_adap0_araddr;
  wire                [31:0] axil_cmac1_awaddr;
  wire                [31:0] axil_cmac1_araddr;
  wire                [31:0] axil_adap1_awaddr;
  wire                [31:0] axil_adap1_araddr;
  wire                [31:0] axil_box1_awaddr;
  wire                [31:0] axil_box1_araddr;
  wire                [31:0] axil_box0_awaddr;
  wire                [31:0] axil_box0_araddr;
  wire                [31:0] axil_smon_awaddr;
  wire                [31:0] axil_smon_araddr;
  wire                [31:0] axil_qcsr_awaddr;
  wire                [31:0] axil_qcsr_araddr;
  wire                [31:0] axil_rdma_awaddr;
  wire                [31:0] axil_rdma_araddr;

  // 定义Crossbar输出的向量化AXI信号
  // 连接到 AXI Crossbar 向量信号
  // ... (为所有AXI通道信号定义向量化的wire)
  wire  [1*C_NUM_SLAVES-1:0] axil_awvalid;
  wire [32*C_NUM_SLAVES-1:0] axil_awaddr;
  wire  [1*C_NUM_SLAVES-1:0] axil_awready;
  wire  [1*C_NUM_SLAVES-1:0] axil_wvalid;
  wire [32*C_NUM_SLAVES-1:0] axil_wdata;
  wire  [1*C_NUM_SLAVES-1:0] axil_wready;
  wire  [1*C_NUM_SLAVES-1:0] axil_bvalid;
  wire  [2*C_NUM_SLAVES-1:0] axil_bresp;
  wire  [1*C_NUM_SLAVES-1:0] axil_bready;
  wire  [1*C_NUM_SLAVES-1:0] axil_arvalid;
  wire [32*C_NUM_SLAVES-1:0] axil_araddr;
  wire  [1*C_NUM_SLAVES-1:0] axil_arready;
  wire  [1*C_NUM_SLAVES-1:0] axil_rvalid;
  wire [32*C_NUM_SLAVES-1:0] axil_rdata;
  wire  [2*C_NUM_SLAVES-1:0] axil_rresp;
  wire  [1*C_NUM_SLAVES-1:0] axil_rready;

  // Adjust AXI-Lite address so that each slave can assume a base address of 0x0
  // --- 核心逻辑 1: 地址平移 (Address Translation) ---
  // `getvec(width, index)` 宏
  // **功能**：生成位向量的范围选择表达式。  
  // **语法**：`(index)*(width) +: (width)`  
  // **参数**：
  // • `width`：每个数据单元的位数；32 每个寄存器都是32位
  // • `index`：数据单元的索引（从0开始）；索引就是定义的0-10设备
  // 作用: 将系统级的绝对地址, 转换为每个从模块自己的、从0开始的相对地址。
  // 例如: 当访问地址0x1004时, Crossbar会将其路由给QDMA (基地址0x1000)。
  // 这行代码会计算出QDMA实际收到的地址是 0x1004 - 0x1000 = 0x4。
  // 这样做极大地增强了从设备IP核的可重用性。
   // ... (为每个从设备都执行地址平移操作)
  assign axil_scfg_awaddr                      = axil_awaddr[`getvec(32, C_SCFG_INDEX)] - C_SCFG_BASE_ADDR;
  assign axil_scfg_araddr                      = axil_araddr[`getvec(32, C_SCFG_INDEX)] - C_SCFG_BASE_ADDR;
  assign axil_qdma_awaddr                      = axil_awaddr[`getvec(32, C_QDMA_INDEX)] - C_QDMA_BASE_ADDR;
  assign axil_qdma_araddr                      = axil_araddr[`getvec(32, C_QDMA_INDEX)] - C_QDMA_BASE_ADDR;
  assign axil_qcsr_awaddr                      = axil_awaddr[`getvec(32, C_QCSR_INDEX)] - C_QCSR_BASE_ADDR;
  assign axil_qcsr_araddr                      = axil_araddr[`getvec(32, C_QCSR_INDEX)] - C_QCSR_BASE_ADDR;
  assign axil_cmac0_awaddr                     = axil_awaddr[`getvec(32, C_CMAC0_INDEX)] - C_CMAC0_BASE_ADDR;
  assign axil_cmac0_araddr                     = axil_araddr[`getvec(32, C_CMAC0_INDEX)] - C_CMAC0_BASE_ADDR;
  assign axil_adap0_awaddr                     = axil_awaddr[`getvec(32, C_ADAP0_INDEX)] - C_ADAP0_BASE_ADDR;
  assign axil_adap0_araddr                     = axil_araddr[`getvec(32, C_ADAP0_INDEX)] - C_ADAP0_BASE_ADDR;
  assign axil_cmac1_awaddr                     = axil_awaddr[`getvec(32, C_CMAC1_INDEX)] - C_CMAC1_BASE_ADDR;
  assign axil_cmac1_araddr                     = axil_araddr[`getvec(32, C_CMAC1_INDEX)] - C_CMAC1_BASE_ADDR;
  assign axil_adap1_awaddr                     = axil_awaddr[`getvec(32, C_ADAP1_INDEX)] - C_ADAP1_BASE_ADDR;
  assign axil_adap1_araddr                     = axil_araddr[`getvec(32, C_ADAP1_INDEX)] - C_ADAP1_BASE_ADDR;
  assign axil_smon_awaddr                      = axil_awaddr[`getvec(32, C_SMON_INDEX)]  - C_SMON_BASE_ADDR;
  assign axil_smon_araddr                      = axil_araddr[`getvec(32, C_SMON_INDEX)] - C_SMON_BASE_ADDR;
  assign axil_rdma_awaddr                      = axil_awaddr[`getvec(32, C_RDMA_INDEX)] - C_RDMA_BASE_ADDR;
  assign axil_rdma_araddr                      = axil_araddr[`getvec(32, C_RDMA_INDEX)] - C_RDMA_BASE_ADDR;
  assign axil_box1_awaddr                      = axil_awaddr[`getvec(32, C_BOX1_INDEX)] - C_BOX1_BASE_ADDR;
  assign axil_box1_araddr                      = axil_araddr[`getvec(32, C_BOX1_INDEX)] - C_BOX1_BASE_ADDR;
  assign axil_box0_awaddr                      = axil_awaddr[`getvec(32, C_BOX0_INDEX)] - C_BOX0_BASE_ADDR;
  assign axil_box0_araddr                      = axil_araddr[`getvec(32, C_BOX0_INDEX)] - C_BOX0_BASE_ADDR;

   // --- 核心逻辑 2: 信号路由 (Signal Routing) ---
  // 作用: 将Crossbar输出的向量化信号, 连接到模块顶层对应的具名端口上。
  // 这是将Crossbar的通用输出端口, 映射到具体命名的下游模块接口。
  // ... (为scfg模块的所有AXI信号进行连接)
  assign m_axil_scfg_awvalid                   = axil_awvalid[C_SCFG_INDEX];
  assign m_axil_scfg_awaddr                    = axil_scfg_awaddr;
  assign axil_awready[C_SCFG_INDEX]            = m_axil_scfg_awready;
  assign m_axil_scfg_wvalid                    = axil_wvalid[C_SCFG_INDEX];
  assign m_axil_scfg_wdata                     = axil_wdata[`getvec(32, C_SCFG_INDEX)];
  assign axil_wready[C_SCFG_INDEX]             = m_axil_scfg_wready;
  assign axil_bvalid[C_SCFG_INDEX]             = m_axil_scfg_bvalid;
  assign axil_bresp[`getvec(2, C_SCFG_INDEX)]  = m_axil_scfg_bresp;
  assign m_axil_scfg_bready                    = axil_bready[C_SCFG_INDEX];
  assign m_axil_scfg_arvalid                   = axil_arvalid[C_SCFG_INDEX];
  assign m_axil_scfg_araddr                    = axil_scfg_araddr;
  assign axil_arready[C_SCFG_INDEX]            = m_axil_scfg_arready;
  assign axil_rvalid[C_SCFG_INDEX]             = m_axil_scfg_rvalid;
  assign axil_rdata[`getvec(32, C_SCFG_INDEX)] = m_axil_scfg_rdata;
  assign axil_rresp[`getvec(2, C_SCFG_INDEX)]  = m_axil_scfg_rresp;
  assign m_axil_scfg_rready                    = axil_rready[C_SCFG_INDEX];

   // ... (为qdma模块的所有AXI信号进行连接)
  assign m_axil_qdma_awvalid                   = axil_awvalid[C_QDMA_INDEX];
  assign m_axil_qdma_awaddr                    = axil_qdma_awaddr;
  assign axil_awready[C_QDMA_INDEX]            = m_axil_qdma_awready;
  assign m_axil_qdma_wvalid                    = axil_wvalid[C_QDMA_INDEX];
  assign m_axil_qdma_wdata                     = axil_wdata[`getvec(32, C_QDMA_INDEX)];
  assign axil_wready[C_QDMA_INDEX]             = m_axil_qdma_wready;
  assign axil_bvalid[C_QDMA_INDEX]             = m_axil_qdma_bvalid;
  assign axil_bresp[`getvec(2, C_QDMA_INDEX)]  = m_axil_qdma_bresp;
  assign m_axil_qdma_bready                    = axil_bready[C_QDMA_INDEX];
  assign m_axil_qdma_arvalid                   = axil_arvalid[C_QDMA_INDEX];
  assign m_axil_qdma_araddr                    = axil_qdma_araddr;
  assign axil_arready[C_QDMA_INDEX]            = m_axil_qdma_arready;
  assign axil_rvalid[C_QDMA_INDEX]             = m_axil_qdma_rvalid;
  assign axil_rdata[`getvec(32, C_QDMA_INDEX)] = m_axil_qdma_rdata;
  assign axil_rresp[`getvec(2, C_QDMA_INDEX)]  = m_axil_qdma_rresp;
  assign m_axil_qdma_rready                    = axil_rready[C_QDMA_INDEX];

   // ... (为qdma csr模块的所有AXI信号进行连接)
  assign m_axil_qdma_csr_awvalid               = axil_awvalid[C_QCSR_INDEX];
  assign m_axil_qdma_csr_awaddr                = axil_qcsr_awaddr;
  assign axil_awready[C_QCSR_INDEX]            = m_axil_qdma_csr_awready;
  assign m_axil_qdma_csr_wvalid                = axil_wvalid[C_QCSR_INDEX];
  assign m_axil_qdma_csr_wdata                 = axil_wdata[`getvec(32, C_QCSR_INDEX)];
  assign axil_wready[C_QCSR_INDEX]             = m_axil_qdma_csr_wready;
  assign axil_bvalid[C_QCSR_INDEX]             = m_axil_qdma_csr_bvalid;
  assign axil_bresp[`getvec(2, C_QCSR_INDEX)]  = m_axil_qdma_csr_bresp;
  assign m_axil_qdma_csr_bready                = axil_bready[C_QCSR_INDEX];
  assign m_axil_qdma_csr_arvalid               = axil_arvalid[C_QCSR_INDEX];
  assign m_axil_qdma_csr_araddr                = axil_qcsr_araddr;
  assign axil_arready[C_QCSR_INDEX]            = m_axil_qdma_csr_arready;
  assign axil_rvalid[C_QCSR_INDEX]             = m_axil_qdma_csr_rvalid;
  assign axil_rdata[`getvec(32, C_QCSR_INDEX)] = m_axil_qdma_csr_rdata;
  assign axil_rresp[`getvec(2, C_QCSR_INDEX)]  = m_axil_qdma_csr_rresp;
  assign m_axil_qdma_csr_rready                = axil_rready[C_QCSR_INDEX];

  if (NUM_CMAC_PORT == 1) begin
     // ... (为cmac模块的所有AXI信号进行连接)
    assign m_axil_cmac_awvalid                    = axil_awvalid[C_CMAC0_INDEX];
    assign m_axil_cmac_awaddr                     = axil_cmac0_awaddr;
    assign axil_awready[C_CMAC0_INDEX]            = m_axil_cmac_awready;
    assign m_axil_cmac_wvalid                     = axil_wvalid[C_CMAC0_INDEX];
    assign m_axil_cmac_wdata                      = axil_wdata[`getvec(32, C_CMAC0_INDEX)];
    assign axil_wready[C_CMAC0_INDEX]             = m_axil_cmac_wready;
    assign axil_bvalid[C_CMAC0_INDEX]             = m_axil_cmac_bvalid;
    assign axil_bresp[`getvec(2, C_CMAC0_INDEX)]  = m_axil_cmac_bresp;
    assign m_axil_cmac_bready                     = axil_bready[C_CMAC0_INDEX];
    assign m_axil_cmac_arvalid                    = axil_arvalid[C_CMAC0_INDEX];
    assign m_axil_cmac_araddr                     = axil_cmac0_araddr;
    assign axil_arready[C_CMAC0_INDEX]            = m_axil_cmac_arready;
    assign axil_rvalid[C_CMAC0_INDEX]             = m_axil_cmac_rvalid;
    assign axil_rdata[`getvec(32, C_CMAC0_INDEX)] = m_axil_cmac_rdata;
    assign axil_rresp[`getvec(2, C_CMAC0_INDEX)]  = m_axil_cmac_rresp;
    assign m_axil_cmac_rready                     = axil_rready[C_CMAC0_INDEX];
     
    // ... (为adap模块的所有AXI信号进行连接)
    assign m_axil_adap_awvalid                    = axil_awvalid[C_ADAP0_INDEX];
    assign m_axil_adap_awaddr                     = axil_adap0_awaddr;
    assign axil_awready[C_ADAP0_INDEX]            = m_axil_adap_awready;
    assign m_axil_adap_wvalid                     = axil_wvalid[C_ADAP0_INDEX];
    assign m_axil_adap_wdata                      = axil_wdata[`getvec(32, C_ADAP0_INDEX)];
    assign axil_wready[C_ADAP0_INDEX]             = m_axil_adap_wready;
    assign axil_bvalid[C_ADAP0_INDEX]             = m_axil_adap_bvalid;
    assign axil_bresp[`getvec(2, C_ADAP0_INDEX)]  = m_axil_adap_bresp;
    assign m_axil_adap_bready                     = axil_bready[C_ADAP0_INDEX];
    assign m_axil_adap_arvalid                    = axil_arvalid[C_ADAP0_INDEX];
    assign m_axil_adap_araddr                     = axil_adap0_araddr;
    assign axil_arready[C_ADAP0_INDEX]            = m_axil_adap_arready;
    assign axil_rvalid[C_ADAP0_INDEX]             = m_axil_adap_rvalid;
    assign axil_rdata[`getvec(32, C_ADAP0_INDEX)] = m_axil_adap_rdata;
    assign axil_rresp[`getvec(2, C_ADAP0_INDEX)]  = m_axil_adap_rresp;
    assign m_axil_adap_rready                     = axil_rready[C_ADAP0_INDEX];

    // Sink for unused CMAC1 register path
    // 对于未使用的端口 (CMAC1, ADAP1), 连接到一个“AXI黑洞”模块 (axi_lite_slave sink)。
    // 这个模块会接收所有请求并给出合法响应, 避免了AXI主端口悬空导致的协议错误。
    // 这是一个非常好的设计实践。
    // ... (CMAC1的连接)
    axi_lite_slave #(
      .REG_ADDR_W (13),
      .REG_PREFIX (16'hC100)
    ) cmac1_reg_inst (
      .s_axil_awvalid (axil_awvalid[C_CMAC1_INDEX]),
      .s_axil_awaddr  (axil_cmac1_awaddr),
      .s_axil_awready (axil_awready[C_CMAC1_INDEX]),
      .s_axil_wvalid  (axil_wvalid[C_CMAC1_INDEX]),
      .s_axil_wdata   (axil_wdata[`getvec(32, C_CMAC1_INDEX)]),
      .s_axil_wready  (axil_wready[C_CMAC1_INDEX]),
      .s_axil_bvalid  (axil_bvalid[C_CMAC1_INDEX]),
      .s_axil_bresp   (axil_bresp[`getvec(2, C_CMAC1_INDEX)]),
      .s_axil_bready  (axil_bready[C_CMAC1_INDEX]),
      .s_axil_arvalid (axil_arvalid[C_CMAC1_INDEX]),
      .s_axil_araddr  (axil_cmac1_araddr),
      .s_axil_arready (axil_arready[C_CMAC1_INDEX]),
      .s_axil_rvalid  (axil_rvalid[C_CMAC1_INDEX]),
      .s_axil_rdata   (axil_rdata[`getvec(32, C_CMAC1_INDEX)]),
      .s_axil_rresp   (axil_rresp[`getvec(2, C_CMAC1_INDEX)]),
      .s_axil_rready  (axil_rready[C_CMAC1_INDEX]),

      .aresetn        (aresetn),
      .aclk           (aclk)
    );

    // Sink for unused ADAP1 register path
    // ... (ADAP1的连接)
    axi_lite_slave #(
      .REG_ADDR_W (13),
      .REG_PREFIX (16'hC100)
    ) adap1_reg_inst (
      .s_axil_awvalid (axil_awvalid[C_ADAP1_INDEX]),
      .s_axil_awaddr  (axil_adap1_awaddr),
      .s_axil_awready (axil_awready[C_ADAP1_INDEX]),
      .s_axil_wvalid  (axil_wvalid[C_ADAP1_INDEX]),
      .s_axil_wdata   (axil_wdata[`getvec(32, C_ADAP1_INDEX)]),
      .s_axil_wready  (axil_wready[C_ADAP1_INDEX]),
      .s_axil_bvalid  (axil_bvalid[C_ADAP1_INDEX]),
      .s_axil_bresp   (axil_bresp[`getvec(2, C_ADAP1_INDEX)]),
      .s_axil_bready  (axil_bready[C_ADAP1_INDEX]),
      .s_axil_arvalid (axil_arvalid[C_ADAP1_INDEX]),
      .s_axil_araddr  (axil_adap1_araddr),
      .s_axil_arready (axil_arready[C_ADAP1_INDEX]),
      .s_axil_rvalid  (axil_rvalid[C_ADAP1_INDEX]),
      .s_axil_rdata   (axil_rdata[`getvec(32, C_ADAP1_INDEX)]),
      .s_axil_rresp   (axil_rresp[`getvec(2, C_ADAP1_INDEX)]),
      .s_axil_rready  (axil_rready[C_ADAP1_INDEX]),

      .aresetn        (aresetn),
      .aclk           (aclk)
    );
  end
  else begin
    // 如果有多个CMAC (例如 NUM_CMAC_PORT = 2), 则将向量化端口连接到对应的Crossbar输出。
    // ... (CMAC0的连接)
    assign m_axil_cmac_awvalid[0]                 = axil_awvalid[C_CMAC0_INDEX];
    assign m_axil_cmac_awaddr[`getvec(32, 0)]     = axil_cmac0_awaddr;
    assign axil_awready[C_CMAC0_INDEX]            = m_axil_cmac_awready[0];
    assign m_axil_cmac_wvalid[0]                  = axil_wvalid[C_CMAC0_INDEX];
    assign m_axil_cmac_wdata[`getvec(32, 0)]      = axil_wdata[`getvec(32, C_CMAC0_INDEX)];
    assign axil_wready[C_CMAC0_INDEX]             = m_axil_cmac_wready[0];
    assign axil_bvalid[C_CMAC0_INDEX]             = m_axil_cmac_bvalid[0];
    assign axil_bresp[`getvec(2, C_CMAC0_INDEX)]  = m_axil_cmac_bresp[`getvec(2, 0)];
    assign m_axil_cmac_bready[0]                  = axil_bready[C_CMAC0_INDEX];
    assign m_axil_cmac_arvalid[0]                 = axil_arvalid[C_CMAC0_INDEX];
    assign m_axil_cmac_araddr[`getvec(32, 0)]     = axil_cmac0_araddr;
    assign axil_arready[C_CMAC0_INDEX]            = m_axil_cmac_arready[0];
    assign axil_rvalid[C_CMAC0_INDEX]             = m_axil_cmac_rvalid[0];
    assign axil_rdata[`getvec(32, C_CMAC0_INDEX)] = m_axil_cmac_rdata[`getvec(32, 0)];
    assign axil_rresp[`getvec(2, C_CMAC0_INDEX)]  = m_axil_cmac_rresp[`getvec(2, 0)];
    assign m_axil_cmac_rready[0]                  = axil_rready[C_CMAC0_INDEX];

    // ... (ADAP0的连接)
    assign m_axil_adap_awvalid[0]                 = axil_awvalid[C_ADAP0_INDEX];
    assign m_axil_adap_awaddr[`getvec(32, 0)]     = axil_adap0_awaddr;
    assign axil_awready[C_ADAP0_INDEX]            = m_axil_adap_awready[0];
    assign m_axil_adap_wvalid[0]                  = axil_wvalid[C_ADAP0_INDEX];
    assign m_axil_adap_wdata[`getvec(32, 0)]      = axil_wdata[`getvec(32, C_ADAP0_INDEX)];
    assign axil_wready[C_ADAP0_INDEX]             = m_axil_adap_wready[0];
    assign axil_bvalid[C_ADAP0_INDEX]             = m_axil_adap_bvalid[0];
    assign axil_bresp[`getvec(2, C_ADAP0_INDEX)]  = m_axil_adap_bresp[`getvec(2, 0)];
    assign m_axil_adap_bready[0]                  = axil_bready[C_ADAP0_INDEX];
    assign m_axil_adap_arvalid[0]                 = axil_arvalid[C_ADAP0_INDEX];
    assign m_axil_adap_araddr[`getvec(32, 0)]     = axil_adap0_araddr;
    assign axil_arready[C_ADAP0_INDEX]            = m_axil_adap_arready[0];
    assign axil_rvalid[C_ADAP0_INDEX]             = m_axil_adap_rvalid[0];
    assign axil_rdata[`getvec(32, C_ADAP0_INDEX)] = m_axil_adap_rdata[`getvec(32, 0)];
    assign axil_rresp[`getvec(2, C_ADAP0_INDEX)]  = m_axil_adap_rresp[`getvec(2, 0)];
    assign m_axil_adap_rready[0]                  = axil_rready[C_ADAP0_INDEX];

    // ... (CMAC1的连接)
    assign m_axil_cmac_awvalid[1]                 = axil_awvalid[C_CMAC1_INDEX];
    assign m_axil_cmac_awaddr[`getvec(32, 1)]     = axil_cmac1_awaddr;
    assign axil_awready[C_CMAC1_INDEX]            = m_axil_cmac_awready[1];
    assign m_axil_cmac_wvalid[1]                  = axil_wvalid[C_CMAC1_INDEX];
    assign m_axil_cmac_wdata[`getvec(32, 1)]      = axil_wdata[`getvec(32, C_CMAC1_INDEX)];
    assign axil_wready[C_CMAC1_INDEX]             = m_axil_cmac_wready[1];
    assign axil_bvalid[C_CMAC1_INDEX]             = m_axil_cmac_bvalid[1];
    assign axil_bresp[`getvec(2, C_CMAC1_INDEX)]  = m_axil_cmac_bresp[`getvec(2, 1)];
    assign m_axil_cmac_bready[1]                  = axil_bready[C_CMAC1_INDEX];
    assign m_axil_cmac_arvalid[1]                 = axil_arvalid[C_CMAC1_INDEX];
    assign m_axil_cmac_araddr[`getvec(32, 1)]     = axil_cmac1_araddr;
    assign axil_arready[C_CMAC1_INDEX]            = m_axil_cmac_arready[1];
    assign axil_rvalid[C_CMAC1_INDEX]             = m_axil_cmac_rvalid[1];
    assign axil_rdata[`getvec(32, C_CMAC1_INDEX)] = m_axil_cmac_rdata[`getvec(32, 1)];
    assign axil_rresp[`getvec(2, C_CMAC1_INDEX)]  = m_axil_cmac_rresp[`getvec(2, 1)];
    assign m_axil_cmac_rready[1]                  = axil_rready[C_CMAC1_INDEX];

     // ... (ADAP1的连接)
    assign m_axil_adap_awvalid[1]                 = axil_awvalid[C_ADAP1_INDEX];
    assign m_axil_adap_awaddr[`getvec(32, 1)]     = axil_adap1_awaddr;
    assign axil_awready[C_ADAP1_INDEX]            = m_axil_adap_awready[1];
    assign m_axil_adap_wvalid[1]                  = axil_wvalid[C_ADAP1_INDEX];
    assign m_axil_adap_wdata[`getvec(32, 1)]      = axil_wdata[`getvec(32, C_ADAP1_INDEX)];
    assign axil_wready[C_ADAP1_INDEX]             = m_axil_adap_wready[1];
    assign axil_bvalid[C_ADAP1_INDEX]             = m_axil_adap_bvalid[1];
    assign axil_bresp[`getvec(2, C_ADAP1_INDEX)]  = m_axil_adap_bresp[`getvec(2, 1)];
    assign m_axil_adap_bready[1]                  = axil_bready[C_ADAP1_INDEX];
    assign m_axil_adap_arvalid[1]                 = axil_arvalid[C_ADAP1_INDEX];
    assign m_axil_adap_araddr[`getvec(32, 1)]     = axil_adap1_araddr;
    assign axil_arready[C_ADAP1_INDEX]            = m_axil_adap_arready[1];
    assign axil_rvalid[C_ADAP1_INDEX]             = m_axil_adap_rvalid[1];
    assign axil_rdata[`getvec(32, C_ADAP1_INDEX)] = m_axil_adap_rdata[`getvec(32, 1)];
    assign axil_rresp[`getvec(2, C_ADAP1_INDEX)]  = m_axil_adap_rresp[`getvec(2, 1)];
    assign m_axil_adap_rready[1]                  = axil_rready[C_ADAP1_INDEX];
  end

   // ... (BOX1的连接)
  assign m_axil_box1_awvalid                   = axil_awvalid[C_BOX1_INDEX];
  assign m_axil_box1_awaddr                    = axil_box1_awaddr;
  assign axil_awready[C_BOX1_INDEX]            = m_axil_box1_awready;
  assign m_axil_box1_wvalid                    = axil_wvalid[C_BOX1_INDEX];
  assign m_axil_box1_wdata                     = axil_wdata[`getvec(32, C_BOX1_INDEX)];
  assign axil_wready[C_BOX1_INDEX]             = m_axil_box1_wready;
  assign axil_bvalid[C_BOX1_INDEX]             = m_axil_box1_bvalid;
  assign axil_bresp[`getvec(2, C_BOX1_INDEX)]  = m_axil_box1_bresp;
  assign m_axil_box1_bready                    = axil_bready[C_BOX1_INDEX];
  assign m_axil_box1_arvalid                   = axil_arvalid[C_BOX1_INDEX];
  assign m_axil_box1_araddr                    = axil_box1_araddr;
  assign axil_arready[C_BOX1_INDEX]            = m_axil_box1_arready;
  assign axil_rvalid[C_BOX1_INDEX]             = m_axil_box1_rvalid;
  assign axil_rdata[`getvec(32, C_BOX1_INDEX)] = m_axil_box1_rdata;
  assign axil_rresp[`getvec(2, C_BOX1_INDEX)]  = m_axil_box1_rresp;
  assign m_axil_box1_rready                    = axil_rready[C_BOX1_INDEX];
 
  // ... (BOX0的连接)
  assign m_axil_box0_awvalid                   = axil_awvalid[C_BOX0_INDEX];
  assign m_axil_box0_awaddr                    = axil_box0_awaddr;
  assign axil_awready[C_BOX0_INDEX]            = m_axil_box0_awready;
  assign m_axil_box0_wvalid                    = axil_wvalid[C_BOX0_INDEX];
  assign m_axil_box0_wdata                     = axil_wdata[`getvec(32, C_BOX0_INDEX)];
  assign axil_wready[C_BOX0_INDEX]             = m_axil_box0_wready;
  assign axil_bvalid[C_BOX0_INDEX]             = m_axil_box0_bvalid;
  assign axil_bresp[`getvec(2, C_BOX0_INDEX)]  = m_axil_box0_bresp;
  assign m_axil_box0_bready                    = axil_bready[C_BOX0_INDEX];
  assign m_axil_box0_arvalid                   = axil_arvalid[C_BOX0_INDEX];
  assign m_axil_box0_araddr                    = axil_box0_araddr;
  assign axil_arready[C_BOX0_INDEX]            = m_axil_box0_arready;
  assign axil_rvalid[C_BOX0_INDEX]             = m_axil_box0_rvalid;
  assign axil_rdata[`getvec(32, C_BOX0_INDEX)] = m_axil_box0_rdata;
  assign axil_rresp[`getvec(2, C_BOX0_INDEX)]  = m_axil_box0_rresp;
  assign m_axil_box0_rready                    = axil_rready[C_BOX0_INDEX];

  // ... (SMON的连接)
  assign m_axil_smon_awvalid                   = axil_awvalid[C_SMON_INDEX];
  assign m_axil_smon_awaddr                    = axil_smon_awaddr;
  assign axil_awready[C_SMON_INDEX]            = m_axil_smon_awready;
  assign m_axil_smon_wvalid                    = axil_wvalid[C_SMON_INDEX];
  assign m_axil_smon_wdata                     = axil_wdata[`getvec(32, C_SMON_INDEX)];
  assign axil_wready[C_SMON_INDEX]             = m_axil_smon_wready;
  assign axil_bvalid[C_SMON_INDEX]             = m_axil_smon_bvalid;
  assign axil_bresp[`getvec(2, C_SMON_INDEX)]  = m_axil_smon_bresp;
  assign m_axil_smon_bready                    = axil_bready[C_SMON_INDEX];
  assign m_axil_smon_arvalid                   = axil_arvalid[C_SMON_INDEX];
  assign m_axil_smon_araddr                    = axil_smon_araddr;
  assign axil_arready[C_SMON_INDEX]            = m_axil_smon_arready;
  assign axil_rvalid[C_SMON_INDEX]             = m_axil_smon_rvalid;
  assign axil_rdata[`getvec(32, C_SMON_INDEX)] = m_axil_smon_rdata;
  assign axil_rresp[`getvec(2, C_SMON_INDEX)]  = m_axil_smon_rresp;
  assign m_axil_smon_rready                    = axil_rready[C_SMON_INDEX];

  // ... (RDMA的连接)
  assign m_axil_rdma_awvalid                   = axil_awvalid[C_RDMA_INDEX];
  assign m_axil_rdma_awaddr                    = axil_rdma_awaddr;
  assign axil_awready[C_RDMA_INDEX]            = m_axil_rdma_awready;
  assign m_axil_rdma_wvalid                    = axil_wvalid[C_RDMA_INDEX];
  assign m_axil_rdma_wdata                     = axil_wdata[`getvec(32, C_RDMA_INDEX)];
  assign axil_wready[C_RDMA_INDEX]             = m_axil_rdma_wready;
  assign axil_bvalid[C_RDMA_INDEX]             = m_axil_rdma_bvalid;
  assign axil_bresp[`getvec(2, C_RDMA_INDEX)]  = m_axil_rdma_bresp;
  assign m_axil_rdma_bready                    = axil_bready[C_RDMA_INDEX];
  assign m_axil_rdma_arvalid                   = axil_arvalid[C_RDMA_INDEX];
  assign m_axil_rdma_araddr                    = axil_rdma_araddr;
  assign axil_arready[C_RDMA_INDEX]            = m_axil_rdma_arready;
  assign axil_rvalid[C_RDMA_INDEX]             = m_axil_rdma_rvalid;
  assign axil_rdata[`getvec(32, C_RDMA_INDEX)] = m_axil_rdma_rdata;
  assign axil_rresp[`getvec(2, C_RDMA_INDEX)]  = m_axil_rdma_rresp;
  assign m_axil_rdma_rready                    = axil_rready[C_RDMA_INDEX];

  // axi_crossbar
  // --- 核心逻辑 4: 例化AXI Crossbar ---
  // 这是一个由工具生成的、或者手写的1对N的AXI Crossbar (或称为Decoder)。
  // 它的功能是接收一个AXI从接口的请求, 根据地址将其解码, 
  // 并将请求转发到11个AXI主接口向量中的某一个。
  system_config_axi_crossbar xbar_inst (
    .s_axi_awaddr  (s_axil_awaddr),
    .s_axi_awprot  (0),
    .s_axi_awvalid (s_axil_awvalid),
    .s_axi_awready (s_axil_awready),
    .s_axi_wdata   (s_axil_wdata),
    .s_axi_wstrb   (4'hF),
    .s_axi_wvalid  (s_axil_wvalid),
    .s_axi_wready  (s_axil_wready),
    .s_axi_bresp   (s_axil_bresp),
    .s_axi_bvalid  (s_axil_bvalid),
    .s_axi_bready  (s_axil_bready),
    .s_axi_araddr  (s_axil_araddr),
    .s_axi_arprot  (0),
    .s_axi_arvalid (s_axil_arvalid),
    .s_axi_arready (s_axil_arready),
    .s_axi_rdata   (s_axil_rdata),
    .s_axi_rresp   (s_axil_rresp),
    .s_axi_rvalid  (s_axil_rvalid),
    .s_axi_rready  (s_axil_rready),

    .m_axi_awaddr  (axil_awaddr),
    .m_axi_awprot  (),
    .m_axi_awvalid (axil_awvalid),
    .m_axi_awready (axil_awready),
    .m_axi_wdata   (axil_wdata),
    .m_axi_wstrb   (),
    .m_axi_wvalid  (axil_wvalid),
    .m_axi_wready  (axil_wready),
    .m_axi_bresp   (axil_bresp),
    .m_axi_bvalid  (axil_bvalid),
    .m_axi_bready  (axil_bready),
    .m_axi_araddr  (axil_araddr),
    .m_axi_arprot  (),
    .m_axi_arvalid (axil_arvalid),
    .m_axi_arready (axil_arready),
    .m_axi_rdata   (axil_rdata),
    .m_axi_rresp   (axil_rresp),
    .m_axi_rvalid  (axil_rvalid),
    .m_axi_rready  (axil_rready),

    .aclk          (aclk),
    .aresetn       (aresetn)
  );

endmodule: system_config_address_map
