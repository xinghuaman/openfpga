/***********************************************************************************************************************
 * Copyright (C) 2016 Andrew Zonenberg and contributors                                                                *
 *                                                                                                                     *
 * This program is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General   *
 * Public License as published by the Free Software Foundation; either version 2.1 of the License, or (at your option) *
 * any later version.                                                                                                  *
 *                                                                                                                     *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied  *
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for     *
 * more details.                                                                                                       *
 *                                                                                                                     *
 * You should have received a copy of the GNU Lesser General Public License along with this program; if not, you may   *
 * find one here:                                                                                                      *
 * https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt                                                              *
 * or you may search the http://www.gnu.org website for the version 2.1 license, or you may write to the Free Software *
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA                                      *
 **********************************************************************************************************************/
 
module Blinky(out_lfosc_ff, out_lfosc_count);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// I/O declarations
	
	(* LOC = "P20" *)
	output reg out_lfosc_ff = 0;
	
	(* LOC = "P19" *)
	output wire out_lfosc_count;
		
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Oscillators
	
	//The 1730 Hz oscillator
	wire clk_108hz;
	GP_LFOSC #(
		.PWRDN_EN(0),
		.AUTO_PWRDN(0),
		.OUT_DIV(16)
	) lfosc (
		.PWRDN(1'b0),
		.CLKOUT(clk_108hz)
	);
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// LED driven by low-frequency oscillator and post-divider in flipflops
	
	parameter COUNT_DEPTH = 3;
	
	//Shift register
	reg[COUNT_DEPTH-1:0] count = 0;
	
	always @(posedge clk_108hz) begin
		count	<= count + 1'd1;
		if(count == 0)
			out_lfosc_ff	<= ~out_lfosc_ff;
	end
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// LED driven by low-frequency oscillator and post-divider in hard counter
	
	GP_COUNT8 #(
		.RESET_MODE("RISING"),
		.COUNT_TO(7),
		.CLKIN_DIVIDE(1)
	) hard_counter (
		.CLK(clk_108hz),
		.RST(1'b0),
		.OUT(out_lfosc_count)
	);

endmodule
