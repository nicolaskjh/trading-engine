#include "logger/Logger.h"
#include "market_data/MarketDataHandler.h"

#include <iomanip>
#include <sstream>

namespace engine {

MarketDataHandler::MarketDataHandler() {
    subId_ = EventBus::getInstance().subscribe(
        EventType::MARKET_DATA,
        [this](const Event& event) { this->onMarketData(event); }
    );
    Logger::info(LogComponent::MARKET_DATA_HANDLER, "Initialized");
}

MarketDataHandler::~MarketDataHandler() {
    EventBus::getInstance().unsubscribe(subId_);
}

void MarketDataHandler::onMarketData(const Event& event) {
    if (const auto* quote = dynamic_cast<const QuoteEvent*>(&event)) {
        std::ostringstream oss;
        oss << "Quote: " << quote->getSymbol()
            << " Bid: $" << std::fixed << std::setprecision(2) 
            << quote->getBidPrice() << " x " << quote->getBidSize()
            << " | Ask: $" << quote->getAskPrice() << " x " << quote->getAskSize()
            << " | Spread: $" << quote->getSpread()
            << " | Latency: " << event.getAgeInMicroseconds() << "μs";
        Logger::debug(LogComponent::MARKET_DATA, oss.str());
    }
    else if (const auto* trade = dynamic_cast<const TradeEvent*>(&event)) {
        std::ostringstream oss;
        oss << "Trade: " << trade->getSymbol()
            << " Price: $" << std::fixed << std::setprecision(2)
            << trade->getPrice() << " Size: " << trade->getSize()
            << " | Latency: " << event.getAgeInMicroseconds() << "μs";
        Logger::debug(LogComponent::MARKET_DATA, oss.str());
    }
}

} // namespace engine
