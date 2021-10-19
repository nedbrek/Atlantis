#pragma once

#include <string>
class ARegion;
class ARegionList;
class JsonReport;

/// Interface for an event
class IEvent
{
public:
	virtual ~IEvent() {}

	virtual std::string str(ARegionList &regions) const = 0;
	virtual void writeJson(JsonReport &of, ARegionList &regions) const = 0;
};

/// Just a string
class StrEvent : public IEvent
{
public:
	explicit StrEvent(const std::string &str): str_(str) {}

	std::string str(ARegionList &) const override { return str_; }

	void writeJson(JsonReport &of, ARegionList &regions) const override;

protected:
	std::string str_; ///< raw string
};

/// Unit collecting taxes in region
class TaxEvent : public IEvent
{
public:
	TaxEvent(const std::string &uname, int amt, ARegion *reg);

	std::string str(ARegionList &regions) const override;
	void writeJson(JsonReport &of, ARegionList &regions) const override;

protected:
	std::string uname_; ///< unit name
	int amt_;           ///< amount of taxes
	ARegion *reg_;      ///< region
};

/// Unit giving items to another unit
class GiveEvent : public IEvent
{
public:
	GiveEvent(const std::string &gname, int inum, int amt, const std::string &tname, bool reversed);

	std::string str(ARegionList &regions) const override;
	void writeJson(JsonReport &of, ARegionList &regions) const override;

protected:
	std::string gname_; ///< giver
	int inum_;          ///< item id
	int amt_;           ///< count
	std::string tname_; ///< target
	bool reversed_;     ///< receive instead of give
};

/// Unit producing items
class ProduceEvent : public IEvent
{
public:
	ProduceEvent(const std::string &uname, int inum, int amt, ARegion *reg);

	std::string str(ARegionList &regions) const override;
	void writeJson(JsonReport &of, ARegionList &regions) const override;

protected:
	std::string uname_; ///< producing unit
	int inum_;          ///< item index
	int amt_;           ///< amount produced
	ARegion *reg_;      ///< region produced in
};

