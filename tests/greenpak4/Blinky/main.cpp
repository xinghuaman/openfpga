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
 
#include "../../../src/greenpak4/Greenpak4.h"
#include <stdio.h>

int main(int /*argc*/, char* /*argv*/[])
{
	//Create the device
	Greenpak4Device device(Greenpak4Device::GREENPAK4_SLG46620);
	
	//Pull pin 3 high
	Greenpak4IOB* iob = device.GetIOB(3);
	Greenpak4BitstreamEntity* vdd = device.GetPowerRail(iob->GetMatrix(), true);
	iob->SetOutputEnable(vdd);
	iob->SetOutputSignal(vdd);
	
	//Pull pins 5-7-9-10 low
	unsigned int pins[] = {5, 7, 9, 10};
	for(auto pin : pins)
	{
		iob = device.GetIOB(pin);
		iob->SetPullStrength(Greenpak4IOB::PULL_10K);
		iob->SetPullDirection(Greenpak4IOB::PULL_DOWN);
	}
	
	//Write the bitstream
	device.WriteToFile("/tmp/Blinky-bits.txt");
	
	return 0;
}
