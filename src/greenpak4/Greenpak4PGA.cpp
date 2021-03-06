/***********************************************************************************************************************
 * Copyright (C) 2016-2017 Andrew Zonenberg and contributors                                                           *
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

#include <log.h>
#include <Greenpak4.h>

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

Greenpak4PGA::Greenpak4PGA(
		Greenpak4Device* device,
		unsigned int cbase)
		: Greenpak4BitstreamEntity(device, 0, -1, -1, cbase)
		, m_vinp(device->GetGround())
		, m_vinn(device->GetGround())
		, m_vinsel(device->GetPower())
		, m_gain(100)
		, m_inputMode(MODE_SINGLE)
		, m_hasNonADCLoads(false)
{
}

Greenpak4PGA::~Greenpak4PGA()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

string Greenpak4PGA::GetDescription() const
{
	return "PGA0";
}

vector<string> Greenpak4PGA::GetInputPorts() const
{
	vector<string> r;
	//no general fabric inputs
	return r;
}

void Greenpak4PGA::SetInput(string port, Greenpak4EntityOutput src)
{
	if(port == "VIN_P")
		m_vinp = src;
	else if(port == "VIN_N")
		m_vinn = src;
	else if(port == "VIN_SEL")
		m_vinsel = src;

	//ignore anything else silently (should not be possible since synthesis would error out)
}

Greenpak4EntityOutput Greenpak4PGA::GetInput(string port) const
{
	if(port == "VIN_P")
		return m_vinp;
	else if(port == "VIN_N")
		return m_vinn;
	else if(port == "VIN_SEL")
		return m_vinsel;
	else
		return Greenpak4EntityOutput(NULL);
}

vector<string> Greenpak4PGA::GetOutputPorts() const
{
	vector<string> r;
	//VOUT is not general fabric routing
	return r;
}

unsigned int Greenpak4PGA::GetOutputNetNumber(string /*port*/)
{
	//no general fabric outputs
	return -1;
}

string Greenpak4PGA::GetPrimitiveName() const
{
	return "GP_PGA";
}

map<string, string> Greenpak4PGA::GetParameters() const
{
	map<string, string> params;

	char tmp[128];
	snprintf(tmp, sizeof(tmp), "%.2f", m_gain * 0.01f);
	//params["GAIN"] = tmp;
	LogWarning("PGA not saving gain parameter due to Yosys bug\n");

	switch(m_inputMode)
	{
		case MODE_SINGLE:
			params["INPUT_MODE"] = "\"SINGLE\"";
			break;

		case MODE_DIFF:
			params["INPUT_MODE"] = "\"DIFF\"";
			break;

		case MODE_PDIFF:
			params["INPUT_MODE"] = "\"PDIFF\"";
			break;
	}

	return params;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

bool Greenpak4PGA::IsUsed()
{
	return HasLoadsOnPort("VOUT");
}

bool Greenpak4PGA::CommitChanges()
{
	//Get our cell, or bail if we're unassigned
	auto ncell = dynamic_cast<Greenpak4NetlistCell*>(GetNetlistEntity());
	if(ncell == NULL)
		return true;

	if(ncell->HasParameter("GAIN"))
	{
		//convert to integer so we can sanity check more easily
		float gain = atof(ncell->m_parameters["GAIN"].c_str());
		int igain = gain*100;

		switch(igain)
		{
			case 25:
			case 50:
			case 100:
			case 200:
			case 400:
			case 800:
			case 1600:
				m_gain = igain;
				break;

			default:
				LogError("PGA GAIN must be 0.25, 0.25, 1, 2, 4, 8, 16\n");
				return false;
		}
	}

	if(ncell->HasParameter("INPUT_MODE"))
	{
		string mode = ncell->m_parameters["INPUT_MODE"];

		if(mode == "SINGLE")
			m_inputMode = MODE_SINGLE;
		else if(mode == "DIFF")
			m_inputMode = MODE_DIFF;
		else if(mode == "PDIFF")
			m_inputMode = MODE_PDIFF;

		else
		{
			LogError("PGA INPUT_MODE must be SINGLE, DIFF, or PDIFF\n");
			return false;
		}
	}

	//See if we have any loads other than the ADC
	m_hasNonADCLoads = false;
	if(ncell && (ncell->m_connections.find("VOUT") != ncell->m_connections.end()) )
	{
		auto node = ncell->m_connections["VOUT"][0];

		for(auto point : node->m_nodeports)
		{
			if(point.m_cell != ncell)
				m_hasNonADCLoads = true;
		}

		if(!node->m_ports.empty())
			m_hasNonADCLoads = true;
	}

	return true;
}

bool Greenpak4PGA::Load(bool* bitstream)
{
	//TODO: read config bit 0

	//If input mux is disabled, vin_sel is vdd
	if(bitstream[m_configBase + 1] == false)
		m_vinsel = m_device->GetPowerRail(true);

	//If input mux is enabled, what do we do here? Not yet supported

	//Read the mode
	int mode = 0;
	if(bitstream[m_configBase + 7])
		mode |= 1;
	if(bitstream[m_configBase + 2])
		mode |= 2;
	switch(mode)
	{
		case 0:
			m_inputMode = MODE_SINGLE;
			break;

		case 2:
			m_inputMode = MODE_DIFF;
			break;

		case 3:
			m_inputMode = MODE_PDIFF;
			break;

		default:
			LogError("Don't know how to handle this PGA mode\n");
			return false;
	}

	//Set the gain
	int gain = 0;
	if(bitstream[m_configBase + 3])
		gain |= 1;
	if(bitstream[m_configBase + 4])
		gain |= 2;
	if(bitstream[m_configBase + 5])
		gain |= 4;
	switch(gain)
	{
		case 0:
			m_gain = 25;
			break;
		case 1:
			m_gain = 50;
			break;
		case 2:
			m_gain = 100;
			break;
		case 3:
			m_gain = 200;
			break;
		case 4:
			m_gain = 400;
			break;
		case 5:
			m_gain = 800;
			break;
		case 6:
			m_gain = 1600;
			break;

		default:
			LogError("Bitstream calls for 32x PGA gain but this isn't supported in current silicon rev\n");
			return false;
	}

	//Ignore force-on bits when reading, we'll recompute this when exporting a bitstream.
	//They don't matter when exporting a netlist

	return true;
}

bool Greenpak4PGA::Save(bool* bitstream)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// INPUT BUS

	//no digital inputs

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Configuration

	//TODO: set true if input came from DAC0, false from anything else
	bitstream[m_configBase + 0] = false;

	//If vinsel is a power rail, disable the input mux
	if(m_vinsel.IsPowerRail())
	{
		if(m_vinsel.GetPowerRailValue() == false)
		{
			LogError("PGA VIN_SEL must be connected to IOB or Vdd\n");
			return false;
		}

		//Pin 16 mux disabled
		else
			bitstream[m_configBase + 1] = false;
	}

	//Assume it's a the correct IOB (no other paths should be routable in the graph)
	else
		bitstream[m_configBase + 1] = true;

	//Set the diff/pdiff mode bits
	switch(m_inputMode)
	{
		case MODE_SINGLE:
			bitstream[m_configBase + 2] = false;
			bitstream[m_configBase + 7] = false;
			break;

		case MODE_DIFF:
			bitstream[m_configBase + 2] = true;
			bitstream[m_configBase + 7] = false;
			break;

		case MODE_PDIFF:
			bitstream[m_configBase + 2] = true;
			bitstream[m_configBase + 7] = true;
			break;
	}

	//Set the gain
	if(m_inputMode == MODE_SINGLE)
	{
		switch(m_gain)
		{
			case 25:
			case 50:
			case 100:
			case 200:
			case 400:
			case 800:
				break;

			default:
				LogError("PGA gain must be 0.25/0.5/1/2/4/8 for single ended inputs\n");
				return false;
		}
	}
	else
	{
		switch(m_gain)
		{
			case 100:
			case 200:
			case 400:
			case 800:
			case 1600:
				break;

			default:
				LogError("PGA gain must be 1/2/4/8/16 for single ended inputs\n");
				return false;
		}
	}

	switch(m_gain)
	{
		case 25:
			bitstream[m_configBase + 5] = false;
			bitstream[m_configBase + 4] = false;
			bitstream[m_configBase + 3] = false;
			break;

		case 50:
			bitstream[m_configBase + 5] = false;
			bitstream[m_configBase + 4] = false;
			bitstream[m_configBase + 3] = true;
			break;

		case 100:
			bitstream[m_configBase + 5] = false;
			bitstream[m_configBase + 4] = true;
			bitstream[m_configBase + 3] = false;
			break;

		case 200:
			bitstream[m_configBase + 5] = false;
			bitstream[m_configBase + 4] = true;
			bitstream[m_configBase + 3] = true;
			break;

		case 400:
			bitstream[m_configBase + 5] = true;
			bitstream[m_configBase + 4] = false;
			bitstream[m_configBase + 3] = false;
			break;

		case 800:
			bitstream[m_configBase + 5] = true;
			bitstream[m_configBase + 4] = false;
			bitstream[m_configBase + 3] = true;
			break;

		case 1600:
			bitstream[m_configBase + 5] = true;
			bitstream[m_configBase + 4] = true;
			bitstream[m_configBase + 3] = false;
			break;

		/*
		//Present in early datasheet revisions but not supported in GreenPak Designer
		//According to discussions w/ Silego engineers, it is "not available" - possible silicon bug?
		case 3200:
			bitstream[m_configBase + 5] = true;
			bitstream[m_configBase + 4] = true;
			bitstream[m_configBase + 3] = true;
			break;
		*/
	}

	//Set the power-on signal if we have any loads other than the ADC

	//Force the PGA on
	bitstream[m_configBase + 6] = m_hasNonADCLoads;

	//Force the ADC on
	//Note that other logic can force this too, so don't write 0 if we don't need it ourself
	if(m_hasNonADCLoads)
		bitstream[m_configBase + 70] = true;

	//Enable the PGA output to non-ADC loads
	bitstream[m_configBase + 71] = m_hasNonADCLoads;

	return true;
}
