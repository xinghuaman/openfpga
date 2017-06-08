/*
Copyright (c) 2016-2017, Robert Ou <rqou@robertou.com> and contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

//! # xc2bit: A library for working with Xilinx Coolrunner-II bitstreams
//!
//! xc2bit is a library for reading and writing bitstreams for the Xilinx Coolrunner-II family of CPLD devices.
//!
//! This project is the result of a reverse-engineering effort involving a combination of [imaging physical
//! CPLD devices](http://siliconexposed.blogspot.com/2014/03/getting-my-feet-wet-with-invasive.html) and black-box
//! reverse-engineering of generated .jed files. It is not an official project of Xilinx, Inc. and is not
//! affiliated or endorsed by Xilinx, Inc.
//!
//! Logically, a Coolrunner-II CPLD contains the following major blocks: function blocks (occasionally abbreviated
//! to FBs), a global interconnect (occasionally referred to as the ZIA or the AIM), and input/output blocks
//! (occasionally abbreviated to IOBs). Function blocks are further divided into the PLA (programmable logic array,
//! a matrix of AND and OR gates) and macrocells. In the Coolrunner-II architecture, macrocells also contain
//! an XOR gate and a register. The global interconnect accepts inputs from IOBs and function blocks and connects these
//! inputs into the PLA of each function block. IOBs also have direct connections to a corresponding macrocell in
//! a function block. (The reverse is not always true - on larger devices, there are macrocells that are not connected
//! to IOBs.) As a special exception, the smallest 32-macrocell devices also have one single input-only pin that is
//! connected directly into the global interconnect and does not have a corresponding macrocell.

/// The number of inputs from the ZIA interconnect into the AND gate section of each PLA.
/// This is an unchangeable property of the architecture of the CPLD.
pub const INPUTS_PER_ANDTERM: usize = 40;
/// The number of AND gates in each PLA. This is also the number of inputs into each OR gate in the PLA.
/// This is an unchangeable property of the architecture of the CPLD.
pub const ANDTERMS_PER_FB: usize = 56;
/// The number of macrocells in each function block. This is also the number of OR gates in each PLA.
/// This is an unchangeable property of the architecture of the CPLD.
pub const MCS_PER_FB: usize = 16;

mod bitstream;
pub use bitstream::{XC2Bitstream, XC2BitstreamBits, XC2GlobalNets};

mod fb;
pub use fb::{XC2BistreamFB, get_ctc, get_ctr, get_cts, get_cte, get_pta, get_ptb, get_ptc};

mod iob;
pub use iob::{XC2MCSmallIOB, XC2IOBZIAMode, XC2IOBOBufMode, XC2ExtraIBuf, iob_num_to_fb_ff_num_32,
             fb_ff_num_to_iob_num_32};

mod mc;
pub use mc::{XC2MCFF, XC2MCFFClkSrc, XC2MCFFResetSrc, XC2MCFFSetSrc, XC2MCFFMode, XC2MCFeedbackMode,
             XC2MCXorMode};

mod pla;
pub use pla::{XC2PLAAndTerm, XC2PLAOrTerm};

mod zia;
pub use zia::{XC2ZIARowPiece, XC2ZIAInput, ZIA_BIT_TO_CHOICE_32};

mod jed;
pub use jed::{read_jed, process_jed};
