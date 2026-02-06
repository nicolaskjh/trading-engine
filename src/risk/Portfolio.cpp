#include "risk/Portfolio.h"

namespace engine {

Portfolio::Portfolio()
    : initialCapital_(Config::getDouble("portfolio.initial_capital", 1000000.0))
    , cash_(initialCapital_)
    , orderManager_(std::make_unique<OrderManager>())
    , maxPositionSize_(Config::getDouble("portfolio.max_position_size", 1000000.0))
    , maxPortfolioExposure_(Config::getDouble("portfolio.max_portfolio_exposure", 5000000.0))
{
    // Subscribe to fill events to update cash
    fillSubId_ = EventBus::getInstance().subscribe(
        EventType::FILL,
        [this](const Event& event) { this->onFillEvent(event); }
    );
}

Portfolio::Portfolio(double initialCapital)
    : initialCapital_(initialCapital)
    , cash_(initialCapital)
    , orderManager_(std::make_unique<OrderManager>())
    , maxPositionSize_(Config::getDouble("portfolio.max_position_size", 1000000.0))
    , maxPortfolioExposure_(Config::getDouble("portfolio.max_portfolio_exposure", 5000000.0))
{
    // Subscribe to fill events to update cash
    fillSubId_ = EventBus::getInstance().subscribe(
        EventType::FILL,
        [this](const Event& event) { this->onFillEvent(event); }
    );
}

Portfolio::~Portfolio() {
    EventBus::getInstance().unsubscribe(fillSubId_);
}

bool Portfolio::submitOrder(const std::string& orderId, const std::string& symbol,
                Side side, OrderType type, double price, int64_t quantity,
                const std::unordered_map<std::string, double>& marketPrices) {
    // Pre-trade risk checks (with lock)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!preTradeRiskCheck(symbol, side, price, quantity, marketPrices)) {
            return false;
        }
    }
    
    // Submit order (without lock - this may trigger events that callback to Portfolio)
    orderManager_->submitOrder(orderId, symbol, side, type, price, quantity);
    return true;
}

void Portfolio::cancelOrder(const std::string& orderId) {
    orderManager_->cancelOrder(orderId);
}

double Portfolio::getCash() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return cash_;
}

double Portfolio::getPortfolioValue(const std::unordered_map<std::string, double>& marketPrices) const {
    std::lock_guard<std::mutex> lock(mutex_);
    double unrealizedPnL = orderManager_->getTotalUnrealizedPnL(marketPrices);
    return cash_ + unrealizedPnL;
}

double Portfolio::getRealizedPnL() const {
    return orderManager_->getTotalRealizedPnL();
}

double Portfolio::getUnrealizedPnL(const std::unordered_map<std::string, double>& marketPrices) const {
    return orderManager_->getTotalUnrealizedPnL(marketPrices);
}

double Portfolio::getTotalPnL(const std::unordered_map<std::string, double>& marketPrices) const {
    return getRealizedPnL() + getUnrealizedPnL(marketPrices);
}

double Portfolio::getGrossExposure(const std::unordered_map<std::string, double>& marketPrices) const {
    std::lock_guard<std::mutex> lock(mutex_);
    double exposure = 0.0;
    
    auto positions = orderManager_->getAllPositions();
    for (const auto& position : positions) {
        auto priceIt = marketPrices.find(position->getSymbol());
        if (priceIt != marketPrices.end()) {
            exposure += std::abs(position->getQuantity() * priceIt->second);
        }
    }
    
    return exposure;
}

double Portfolio::getNetExposure(const std::unordered_map<std::string, double>& marketPrices) const {
    std::lock_guard<std::mutex> lock(mutex_);
    double exposure = 0.0;
    
    auto positions = orderManager_->getAllPositions();
    for (const auto& position : positions) {
        auto priceIt = marketPrices.find(position->getSymbol());
        if (priceIt != marketPrices.end()) {
            exposure += position->getQuantity() * priceIt->second;
        }
    }
    
    return exposure;
}

void Portfolio::setMaxPositionSize(double maxSize) {
    std::lock_guard<std::mutex> lock(mutex_);
    maxPositionSize_ = maxSize;
}

double Portfolio::getMaxPositionSize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return maxPositionSize_;
}

void Portfolio::setMaxPortfolioExposure(double maxExposure) {
    std::lock_guard<std::mutex> lock(mutex_);
    maxPortfolioExposure_ = maxExposure;
}

double Portfolio::getMaxPortfolioExposure() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return maxPortfolioExposure_;
}

void Portfolio::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    cash_ = initialCapital_;
    orderManager_->clear();
}

bool Portfolio::preTradeRiskCheck(const std::string& symbol, Side side, double price,
                      int64_t quantity, 
                      const std::unordered_map<std::string, double>& marketPrices) const {
    // Calculate order value
    double orderValue = price * quantity;
    
    // Check if we have enough cash for buy orders
    if (side == Side::BUY && orderValue > cash_) {
        return false;  // Insufficient cash
    }
    
    // Calculate what position would be after this order
    auto position = orderManager_->getPosition(symbol);
    int64_t currentQty = position ? position->getQuantity() : 0;
    int64_t newQty = (side == Side::BUY) ? currentQty + quantity : currentQty - quantity;
    double newPositionValue = std::abs(newQty * price);
    
    // Check position size limit
    if (newPositionValue > maxPositionSize_) {
        return false;  // Would exceed position size limit
    }
    
    // Calculate new gross exposure
    double currentExposure = 0.0;
    auto positions = orderManager_->getAllPositions();
    for (const auto& pos : positions) {
        if (pos->getSymbol() == symbol) {
            continue;  // Skip - we'll use new position value
        }
        auto priceIt = marketPrices.find(pos->getSymbol());
        if (priceIt != marketPrices.end()) {
            currentExposure += std::abs(pos->getQuantity() * priceIt->second);
        }
    }
    
    double newExposure = currentExposure + newPositionValue;
    
    // Check portfolio exposure limit
    if (newExposure > maxPortfolioExposure_) {
        return false;  // Would exceed portfolio exposure limit
    }
    
    return true;  // All checks passed
}

void Portfolio::onFillEvent(const Event& event) {
    const auto* fillEvent = dynamic_cast<const FillEvent*>(&event);
    if (!fillEvent) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Calculate trade value
    double tradeValue = fillEvent->getFillPrice() * fillEvent->getFillQuantity();
    
    // Adjust cash based on trade side
    if (fillEvent->getSide() == Side::BUY) {
        // Buying: debit cash
        cash_ -= tradeValue;
    } else if (fillEvent->getSide() == Side::SELL) {
        // Selling: credit cash
        cash_ += tradeValue;
    }
}

} // namespace engine
