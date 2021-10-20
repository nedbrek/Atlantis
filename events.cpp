#include "events.h"
#include "aregion.h"
#include "astring.h"
#include "fileio.h"
#include "items.h"
#include <sstream>

//----------------------------------------------------------------------------
void StrEvent::writeJson(JsonReport &of, ARegionList &) const
{
	of.putStr(str_.c_str());
}

//----------------------------------------------------------------------------
TaxEvent::TaxEvent(const std::string &uname, int amt, ARegion *reg)
: uname_(uname)
, amt_(amt)
, reg_(reg)
{
}

std::string TaxEvent::str(ARegionList &regions) const
{
	std::ostringstream os;
	os << uname_ << ": Collects $" << amt_ << " in taxes in ";
	os << reg_->ShortPrint(&regions) << '.';
	return os.str();
}

void TaxEvent::writeJson(JsonReport &of, ARegionList &regions) const
{
	of.startDict(NULL);
	of.putPairStr("type", "Tax");
	of.putPairStr("name", uname_.c_str());
	of.putPairInt("amount", amt_);

	of.startDict("region");
	reg_->CPrint(&of, &regions, false);
	of.endDict();

	of.endDict();
}

//----------------------------------------------------------------------------
GiveEvent::GiveEvent(const std::string &gname, int inum, int amt, const std::string &tname, bool reversed)
: gname_(gname)
, inum_(inum)
, amt_(amt)
, tname_(tname)
, reversed_(reversed)
{
}

std::string GiveEvent::str(ARegionList &) const
{
	std::ostringstream os;
	if (reversed_)
		os << tname_ << ": Receives " << ItemString(inum_, amt_) << " from " << gname_ << '.';
	else
		os << gname_ << ": Gives " << ItemString(inum_, amt_) << " to " << tname_ << '.';

	return os.str();
}

void GiveEvent::writeJson(JsonReport &of, ARegionList &regions) const
{
	of.startDict(NULL);
	of.putPairStr("type", reversed_ ? "Receive" : "Give");
	of.putPairStr("giver", gname_.c_str());
	of.putPairStr("receiver", tname_.c_str());
	of.putPairStr("abbr", ItemDefs[inum_].abr);
	of.putPairInt("amount", amt_);
	of.endDict();
}

//----------------------------------------------------------------------------
ProduceEvent::ProduceEvent(const std::string &uname, int inum, int amt, ARegion *reg)
: uname_(uname)
, inum_(inum)
, amt_(amt)
, reg_(reg)
{
}

std::string ProduceEvent::str(ARegionList &regions) const
{
	std::ostringstream os;
	os << uname_ << ": Produces " << ItemString(inum_, amt_) << " in " << reg_->ShortPrint(&regions) << '.';

	return os.str();
}

void ProduceEvent::writeJson(JsonReport &of, ARegionList &regions) const
{
	of.startDict(NULL);
	of.putPairStr("type", "Produce");
	of.putPairStr("name", uname_.c_str());
	of.putPairStr("abbr", ItemDefs[inum_].abr);
	of.putPairInt("amount", amt_);

	of.startDict("region");
	reg_->CPrint(&of, &regions, false);
	of.endDict();

	of.endDict();
}

