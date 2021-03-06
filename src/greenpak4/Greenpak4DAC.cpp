/***********************************************************************************************************************
 * Copyright (C) 2017 Andrew Zonenberg and contributors                                                                *
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

Greenpak4DAC::Greenpak4DAC(
	Greenpak4Device* device,
	unsigned int cbase_reg,
	unsigned int cbase_pwr,
	unsigned int cbase_insel,
	unsigned int cbase_aon,
	unsigned int dacnum)
	: Greenpak4BitstreamEntity(device, 0, -1, -1, -1)
		, m_vref(device->GetGround())
		, m_dacnum(dacnum)
		, m_cbaseReg(cbase_reg)
		, m_cbasePwr(cbase_pwr)
		, m_cbaseInsel(cbase_insel)
		, m_cbaseAon(cbase_aon)
{
	for(unsigned int i=0; i<8; i++)
		m_din[i] = device->GetGround();

	//Make our output a dual so we don't infer cross connections when driving reference-output pins
	//(even though we don't have a general fabric output)
	m_dual = new Greenpak4DualEntity(this);
}

Greenpak4DAC::~Greenpak4DAC()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Accessors

bool Greenpak4DAC::IsUsed()
{
	return HasLoadsOnPort("VOUT");
}

string Greenpak4DAC::GetDescription() const
{
	char buf[128];
	snprintf(buf, sizeof(buf), "DAC_%u", m_dacnum);
	return string(buf);
}

vector<string> Greenpak4DAC::GetInputPorts() const
{
	vector<string> r;
	//no general fabric inputs
	return r;
}

void Greenpak4DAC::SetInput(string port, Greenpak4EntityOutput src)
{
	if(port == "VREF")
		m_vref = src;

	else if(port.find("DIN") == 0)
	{
		int b = 0;
		if(1 != sscanf(port.c_str(), "DIN[%d]", &b))
		{
			LogError("Greenpak4DAC: Malformed input name\n");
			return;
		}
		if( (b < 0) || (b >= 8) )
		{
			LogError("Greenpak4DAC: Out of range input index\n");
			return;
		}

		m_din[b] = src;
	}

	//ignore anything else silently (should not be possible since synthesis would error out)
}

Greenpak4EntityOutput Greenpak4DAC::GetInput(string port) const
{
	if(port == "VREF")
		return m_vref;
	else if(port.find("DIN") == 0)
	{
		int b = 0;
		if(1 != sscanf(port.c_str(), "DIN[%d]", &b))
		{
			LogError("Greenpak4DAC: Malformed input name\n");
			return Greenpak4EntityOutput(NULL);
		}
		if( (b < 0) || (b >= 8) )
		{
			LogError("Greenpak4DAC: Out of range input index\n");
			return Greenpak4EntityOutput(NULL);
		}
		return m_din[b];
	}
	else
		return Greenpak4EntityOutput(NULL);
}

vector<string> Greenpak4DAC::GetOutputPorts() const
{
	vector<string> r;
	//no general fabric outputs
	return r;
}

unsigned int Greenpak4DAC::GetOutputNetNumber(string /*port*/)
{
	//no general fabric outputs
	return -1;
}

string Greenpak4DAC::GetPrimitiveName() const
{
	return "GP_DAC";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

bool Greenpak4DAC::CommitChanges()
{
	//No configuration
	return true;
}

bool Greenpak4DAC::Load(bool* bitstream)
{
	//TODO: VREF
	LogWarning("TODO: VREF configuration for Greenpak4DAC\n");

	//If DAC is disabled, set VREF to ground (we're not used)
	if(!bitstream[m_cbasePwr])
	{
		m_vref = m_device->GetGround();
		return true;
	}

	//ignore cbaseAon for now

	if(m_device->GetPart() == Greenpak4Device::GREENPAK4_SLG46140)
		LogError("Greenpak4DAC: not implemented for 46140 yet\n");

	//Input selector

	/*
	//WTF, the config is flipped from DAC0 to DAC1??? (see SLG46620V table 40)
	//This also applies to the SLG46140 (see SLG46140 table 28).
	bool dinPower = (m_din[0].IsPowerRail());
	if(m_dacnum == 0)
		bitstream[m_cbaseInsel] = !dinPower;
	else
		bitstream[m_cbaseInsel] = dinPower;

	//Constant input voltage
	if(dinPower)
	{
		for(unsigned int i=0; i<8; i++)
			bitstream[m_cbaseReg + i] = m_din[i].GetPowerRailValue();
	}

	//Input is coming from DCMP.
	//Rely on the DCMP input mux for this, nothing for us to do.
	//Set the input voltage to zero just so the bitstream is deterministic.
	else
	{
		for(unsigned int i=0; i<8; i++)
			bitstream[m_cbaseReg + i] = false;
	}

	*/

	//TODO: Do our inputs
	LogError("Unimplemented\n");
	return false;
}

bool Greenpak4DAC::Save(bool* bitstream)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// INPUT BUS

	//If VREF is ground, then the DAC is unused, leave config blank
	if(m_vref.IsPowerRail() && !m_vref.GetPowerRailValue())
		return true;

	//VREF must be from a GP_VREF driving 1v0 for now
	//Sanity check that
	//TODO: DAC1 has a 50 mV offset, so the two are NOT equivalent!
	if(!m_vref.IsVoltageReference())
	{
		LogError("DRC: DAC should have a voltage reference driving VREF, but something else was supplied instead\n");
		return false;
	}
	auto v = dynamic_cast<Greenpak4VoltageReference*>(m_vref.GetRealEntity());
	if(!v->IsConstantVoltage() || (v->GetOutputVoltage() != 1000) )
	{
		LogError("DRC: DAC should be driven by a constant 1000 mV reference\n");
		return false;
	}

	//Verify that all 8 bits of each input came from the same entity
	//TODO: verify bit ordering?
	for(int i=1; i<8; i++)
	{
		if(m_din[i].GetRealEntity() != m_din[0].GetRealEntity())
		{
			LogError("All bits of GP_DAC DIN must come from the same source node\n");
			return false;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// CONFIGURATION

	//Apparently DAC1 will die a horrible death if the ADC analog part is on
	//so don't turn it on even though the datasheet kinda implies we might need it?

	//Turn our DAC on
	bitstream[m_cbasePwr] = true;

	//SLG4662x: If we're using DAC1, turn on DAC0
	//This is a legal no-op in other situations.
	//TODO: maybe add a routing preference so that DAC0 is preferred to DAC1 in a single-DAC design
	//(otherwise we're wasting a bit of power)
	bitstream[m_cbaseAon] = true;

	if(m_device->GetPart() == Greenpak4Device::GREENPAK4_SLG46140)
		LogError("Greenpak4DAC: not implemented for 46140 yet\n");

	//Input selector
	//WTF, the config is flipped from DAC0 to DAC1??? (see SLG46620V table 40)
	//This also applies to the SLG46140 (see SLG46140 table 28).
	bool dinPower = (m_din[0].IsPowerRail());
	if(m_dacnum == 0)
		bitstream[m_cbaseInsel] = !dinPower;
	else
		bitstream[m_cbaseInsel] = dinPower;

	//Constant input voltage
	if(dinPower)
	{
		for(unsigned int i=0; i<8; i++)
			bitstream[m_cbaseReg + i] = m_din[i].GetPowerRailValue();
	}

	//Input is coming from DCMP.
	//Rely on the DCMP input mux for this, nothing for us to do.
	//Set the input voltage to zero just so the bitstream is deterministic.
	else
	{
		for(unsigned int i=0; i<8; i++)
			bitstream[m_cbaseReg + i] = false;
	}

	return true;
}
