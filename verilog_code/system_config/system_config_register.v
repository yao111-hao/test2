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
// Address range: 0x0000 - 0x0FFF
// Address width: 12-bit
//
// Register description
// -----------------------------------------------------------------------------
//  Address | Mode |          Description
// -----------------------------------------------------------------------------
//   0x000  |  RO  | Build timestamp register
// -----------------------------------------------------------------------------
//   0x004  |  WO  | System reset register
// -----------------------------------------------------------------------------
//   0x008  |  RO  | System status register
// -----------------------------------------------------------------------------
//   0x00C  |  WO  | Shell reset register
// -----------------------------------------------------------------------------
//   0x010  |  RO  | Shell status register
// -----------------------------------------------------------------------------
//   0x014  |  WO  | User reset register
// -----------------------------------------------------------------------------
//   0x018  |  RO  | User status register
// -----------------------------------------------------------------------------
// *************************************************************************
// --- 寄存器地址映射表 ---
// Address range: 0x0000 - 0x0FFF (地址范围)
// Address width: 12-bit (地址位宽)
//
// Register description
// -----------------------------------------------------------------------------
//  Address | Mode |          Description
// -----------------------------------------------------------------------------
//   0x000  |  RO  | Build timestamp register (构建时间戳寄存器 - 只读)
// -----------------------------------------------------------------------------
//   0x004  |  WO  | System reset register (系统复位寄存器 - 只写)
// -----------------------------------------------------------------------------
//   0x008  |  RO  | System status register (系统状态寄存器 - 只读)
// -----------------------------------------------------------------------------
//   0x00C  |  WO  | Shell reset register (Shell层复位寄存器 - 只写)
// -----------------------------------------------------------------------------
//   0x010  |  RO  | Shell status register (Shell层状态寄存器 - 只读)
// -----------------------------------------------------------------------------
//   0x014  |  WO  | User reset register (用户层复位寄存器 - 只写)
// -----------------------------------------------------------------------------
//   0x018  |  RO  | User status register (用户层状态寄存器 - 只读)
// -----------------------------------------------------------------------------
`timescale 1ns/1ps
module system_config_register #(
   // 参数: 构建时间戳。这是一个非常实用的技巧，用于让软件可以读取硬件的版本信息。
  // 在编译FPGA比特流时，可以通过脚本将当前的日期时间传入这个参数。
  parameter [31:0] BUILD_TIMESTAMP = 32'h01010000
) (
  // --- AXI4-Lite 从接口 ---
  input         s_axil_awvalid,
  input  [31:0] s_axil_awaddr,
  output        s_axil_awready,
  input         s_axil_wvalid,
  input  [31:0] s_axil_wdata,
  output        s_axil_wready,
  output        s_axil_bvalid,
  output  [1:0] s_axil_bresp,
  input         s_axil_bready,
  input         s_axil_arvalid,
  input  [31:0] s_axil_araddr,
  output        s_axil_arready,
  output        s_axil_rvalid,
  output [31:0] s_axil_rdata,
  output  [1:0] s_axil_rresp,
  input         s_axil_rready,

  // --- 复位控制与状态端口 ---
  // 这些端口连接到FPGA设计中的其他模块
  output [31:0] shell_rstn,
  input  [31:0] shell_rst_done,
  output [31:0] user_rstn,
  input  [31:0] user_rst_done,
  // --- 时钟与复位 ---
  input         aclk,   // 全局时钟 (例如 125MHz)
  input         aresetn
);

    // --- 内部参数定义 ---
  localparam C_ADDR_W = 12; // 定义内部使用的地址位宽为12位

  // 寄存器地址常量定义，提高代码可读性和可维护性
  localparam REG_BUILD_TIMESTAMP = 12'h000;
  localparam REG_SYSTEM_RST      = 12'h004;
  localparam REG_SYSTEM_STATUS   = 12'h008;
  localparam REG_SHELL_RST       = 12'h00C;
  localparam REG_SHELL_STATUS    = 12'h010;
  localparam REG_USER_RST        = 12'h014;
  localparam REG_USER_STATUS     = 12'h018;

  // --- 内部寄存器存储 ---
  reg [31:0] reg_build_timestamp; // (未使用, 直接从参数输出)
  reg        reg_system_rst;      // 系统复位触发寄存器
  reg        reg_system_status;   // 系统状态寄存器
  reg [31:0] reg_shell_rst;       // Shell层复位触发寄存器 (位掩码)
  reg [31:0] reg_shell_status;    // Shell层状态寄存器 (位掩码)
  reg [31:0] reg_user_rst;        // User层复位触发寄存器 (位掩码)
  reg [31:0] reg_user_status;     // User层状态寄存器 (位掩码)

 // --- 内部信号定义 ---
  reg [31:0] shell_rst_last;  // 用于检测shell_rst寄存器写操作的上升沿
  reg [31:0] user_rst_last;   // 用于检测user_rst寄存器写操作的上升沿
  reg        system_rst_last; // 用于检测system_rst寄存器写操作的上升沿
  wire       system_rst;      // 单周期的系统复位脉冲
  wire       system_rst_done; // 整个系统复位完成的标志
  
  // 连接到axi_lite_register模块的简单寄存器接口信号
  wire                reg_en;
  wire                reg_we;
  wire [C_ADDR_W-1:0] reg_addr;
  wire         [31:0] reg_din;
  reg          [31:0] reg_dout;

   // --- 例化AXI-Lite到寄存器风格接口的转换模块 ---
  // 这是上一问分析的核心模块，这里是它的实际应用
  axi_lite_register #(
    .CLOCKING_MODE ("common_clock"), // AXI时钟和寄存器逻辑时钟相同
    .ADDR_W        (C_ADDR_W),     // 内部地址位宽为12位
    .DATA_W        (32)            // 数据位宽为32位
  ) axil_reg_inst (
     // AXI-Lite 从接口连接到本模块的顶层端口
    .s_axil_awvalid (s_axil_awvalid),
    .s_axil_awaddr  (s_axil_awaddr[C_ADDR_W-1:0]), // 注意: 截取了地址的低12位
    .s_axil_awready (s_axil_awready),
    .s_axil_wvalid  (s_axil_wvalid),
    .s_axil_wdata   (s_axil_wdata),
    .s_axil_wready  (s_axil_wready),
    .s_axil_bvalid  (s_axil_bvalid),
    .s_axil_bresp   (s_axil_bresp),
    .s_axil_bready  (s_axil_bready),
    .s_axil_arvalid (s_axil_arvalid),
    .s_axil_araddr  (s_axil_araddr[C_ADDR_W-1:0]), // 截取地址的低12位
    .s_axil_arready (s_axil_arready),
    .s_axil_rvalid  (s_axil_rvalid),
    .s_axil_rdata   (s_axil_rdata),
    .s_axil_rresp   (s_axil_rresp),
    .s_axil_rready  (s_axil_rready),

    // 简单的寄存器接口连接到本模块的内部逻辑
    .reg_en         (reg_en), // 寄存器访问使能信号 (读或写)
    .reg_we         (reg_we), // 寄存器写使能信号 (高有效表示写, 低有效表示读)
    .reg_addr       (reg_addr),
    .reg_din        (reg_din),
    .reg_dout       (reg_dout),

    // 时钟和复位都使用同一个源
    .axil_aclk      (aclk), //125M
    .axil_aresetn   (aresetn),
    .reg_clk        (aclk), //125M
    .reg_rstn       (aresetn)
  );
 //***************************************************************************************/
 // --- 读操作逻辑: 根据地址返回不同的寄存器值 ---
  always @(posedge aclk) begin
    if (~aresetn) begin
      reg_dout <= 0;
    end
    else if (reg_en && ~reg_we) begin // 当有读请求时 (reg_en=1, reg_we=0)
      case (reg_addr)
        REG_BUILD_TIMESTAMP: begin
          reg_dout <= BUILD_TIMESTAMP; // 返回编译时设定的时间戳
        end
        REG_SYSTEM_STATUS: begin
          reg_dout <= {31'b0, reg_system_status}; // 返回系统状态
        end
        REG_SHELL_STATUS: begin
          reg_dout <= reg_shell_status; // 返回Shell层状态
        end
        REG_USER_STATUS: begin
          reg_dout <= reg_user_status; // 返回User层状态
        end
        default: begin
          // 如果读取一个无效地址, 返回一个特殊的调试值 "DEADBEEF"。
          // 这能帮助软件开发者快速定位到错误的地址访问。
          reg_dout <= 32'hDEADBEEF;
        end
      endcase
    end
  end

// --- 锁存上一拍的复位寄存器值，用于边沿检测 ---
// 如何将一个可能持续多拍的“电平置位”操作，转换为一个精确的、只持续一拍的“触发脉冲”事件？
// 检测“从0变为1”的那个瞬间，即上升沿。代码正是通过“锁存上一拍的值”来实现这一点的。
// 逻辑表达式 (~last && current) 只有在一种情况下才为真：
// 上一个周期，值是 0 (~shell_rst_last 为真)，并且这个周期，值变成了 1 (reg_shell_rst 为真)。
  always @(posedge aclk) begin
    if (~aresetn) begin
      shell_rst_last  <= 0;
      user_rst_last   <= 0;
      system_rst_last <= 1'b0;
    end
    else begin
      shell_rst_last  <= reg_shell_rst;
      user_rst_last   <= reg_user_rst;
      system_rst_last <= reg_system_rst;
    end
  end

 // --- 复位信号生成逻辑 ---
  // 子模块的复位信号 (低有效) 在以下两种情况被触发(变为高电平):
  // 1. 全局的系统复位 `system_rst` 发生。
  // 2. 对应的复位寄存器位被写入1 (通过检测 `~last && current` 的上升沿)。
  // generate...endgenerate 用于为32个比特分别生成独立的复位逻辑。
  // 全局复位生效或者当前模块复位生效
  generate for (genvar ii = 0; ii < 32; ii = ii + 1) begin
    assign shell_rstn[ii] = ~(system_rst || (~shell_rst_last[ii] && reg_shell_rst[ii]));
    assign user_rstn[ii]  = ~(system_rst || (~user_rst_last[ii] && reg_user_rst[ii]));
  end
  endgenerate

  // 生成单周期的系统复位脉冲
  assign system_rst      = ~system_rst_last && reg_system_rst;
  // 系统复位完成信号: 当所有shell子模块和所有user子模块都报告复位完成后才为高
  assign system_rst_done = (&shell_rst_done) && (&user_rst_done); // '&'是缩减与操作符


  //***************************************************************************************/
  // 写复位寄存器
  // --- 写操作逻辑: 更新各个只写寄存器 ---
  //
  // 没写入一次复位寄存器，对应的状态就会更新，查看状态是否完成，若完成，则停止复位，否则保持
  // 1 系统复位寄存器 (REG_SYSTEM_RST)
  // 这是一个"自清除"寄存器。
  // 写入1触发复位, 复位会一直保持, 直到所有子模块报告完成后(`system_rst_done`), 它才自动清零。
  always @(posedge aclk) begin
    if (~aresetn) begin
      reg_system_rst <= 1'b1; // 上电时强制进行一次系统复位
    end
    else if (reg_en && reg_we && reg_addr == REG_SYSTEM_RST) begin
      reg_system_rst <= 1'b1; // 响应AXI写操作, 触发复位
    end
    else if (system_rst_done) begin
      reg_system_rst <= 0;    // 复位完成后, 自动清零
    end
  end


  // Shell层复位寄存器 (REG_SHELL_RST)
  // 这是一个位掩码寄存器, 每一位对应一个Shell层子模块的复位。
  // 同样是"自清除"逻辑。
  // Shell reset register (write-only)
  //
  // 31:10 - reserved
  // 9     - reset for the adapter of CMAC1
  // 8     - reset for the CMAC subsystem CMAC1
  // 7:6   - reserved
  // 5     - reset for the adapter of CMAC0
  // 4     - reset for the CMAC subsystem CMAC0
  // 3:2   - reserved
  // 1     - reset for the RDMA subsystem
  // 0     - reset for the QDMA subsystem
  // Writing 1 to a bit of this register initiates a submodule-level reset in
  // the shell logic, which lasts until the corresponding submodule is out of
  // reset.  Mapping between bits and submodules are as follows.
  generate for (genvar ii = 0; ii < 32; ii = ii + 1) begin
    always @(posedge aclk) begin
      if (~aresetn) begin
        reg_shell_rst[ii] <= 1'b0;
      end
      // 如果AXI向该寄存器写入, 并且写入数据的对应位为1
      else if (reg_en && reg_we && reg_addr == REG_SHELL_RST && reg_din[ii]) begin
        reg_shell_rst[ii] <= 1'b1; // 触发该子模块的复位
      end
      // 如果该子模块报告复位完成
      else if (shell_rst_done[ii]) begin
        reg_shell_rst[ii] <= 0;    // 自动清除该位的复位触发
      end
    end
  end
  endgenerate

  // User层复位寄存器 (REG_USER_RST)
  // 逻辑与Shell层复位寄存器完全相同, 但控制的是User层的子模块。
  // 将 1 写入该寄存器的位会在用户逻辑中启动子模块级复位，
  // 该复位一直持续到相应的子模块复位完成。
  // 位和子模块之间的映射取决于用户逻辑在顶层的连接方式。
  // User reset register (write-only)
  //
  // Writing 1 to a bit of this register initiates a submodule-level reset in
  // the user logic, which lasts until the corresponding submodule is out of
  // reset.  Mapping between bits and submodules are determined by how the user
  // logic is connected in the top level.
  generate for (genvar ii = 0; ii < 32; ii = ii + 1) begin
    always @(posedge aclk) begin
      if (~aresetn) begin
        reg_user_rst[ii] <= 1'b0;
      end
      else if (reg_en && reg_we && reg_addr == REG_USER_RST && reg_din[ii]) begin
        reg_user_rst[ii] <= 1'b1;
      end
      else if (user_rst_done[ii]) begin
        reg_user_rst[ii] <= 0;
      end
    end
  end
  endgenerate

  //***************************************************************************************/
  // --- 状态寄存器更新逻辑 ---
  // System status register (read-only)
  // 
  // 31:1  - reserved
  // 0     - system reset done
  // 
  // The register shows the system reset status.
  // 系统状态寄存器 (REG_SYSTEM_STATUS)
  always @(posedge aclk) begin
    if (~aresetn) begin
      reg_system_status <= 1'b0; // 复位期间, 状态为 "未完成"
    end
    else if (system_rst) begin
      reg_system_status <= 1'b0; // 系统复位脉冲来临时, 状态清零
    end
    else if (system_rst_done) begin
      reg_system_status <= 1'b1; // 系统复位完成后, 状态置位
    end
  end

  // Shell层状态寄存器 (REG_SHELL_STATUS)
  // Shell status register (read-only)
  //
  // 31:3  - reserved
  // 2     - CMAC subsystem CMAC1 reset done
  // 1     - CMAC subsystem CMAC0 reset done
  // 0     - QDMA subsystem reset done
  //
  // This register shows the reset status of shell submodules.
  generate for (genvar ii = 0; ii < 32; ii = ii + 1) begin
    always @(posedge aclk) begin
      if (~aresetn) begin
        reg_shell_status[ii] <= 1'b0;
      end
      else if (~shell_rstn[ii]) begin // 如果该子模块正在被复位
        reg_shell_status[ii] <= 1'b0;   // 状态为 "未完成"
      end
      else if (shell_rst_done[ii]) begin // 如果该子模块报告复位完成
        reg_shell_status[ii] <= 1'b1;   // 状态为 "已完成"
      end
    end
  end
  endgenerate

   // User层状态寄存器 (REG_USER_STATUS)
  // User status register (read-only)
  //
  // This register shows the reset status of user submodules.  Mapping between
  // bits and submodules are determined by how the user logic is connected in
  // the top level.
  generate for (genvar ii = 0; ii < 32; ii = ii + 1) begin
    always @(posedge aclk) begin
      if (~aresetn) begin
        reg_user_status[ii] <= 1'b0;
      end
      else if (~user_rstn[ii]) begin
        reg_user_status[ii] <= 1'b0;
      end
      else if (user_rst_done[ii]) begin
        reg_user_status[ii] <= 1'b1;
      end
    end
  end
  endgenerate

endmodule: system_config_register
