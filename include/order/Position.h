#pragma once

#include "event/OrderEvent.h"

#include <string>

namespace engine {

/**
 * Position: Tracks holdings for a single symbol.
 * Maintains quantity, average entry price, and P&L calculations.
 */
class Position {
public:
    explicit Position(const std::string& symbol)
        : symbol_(symbol),
          quantity_(0),
          averagePrice_(0.0),
          realizedPnL_(0.0) {}

    // Getters
    const std::string& getSymbol() const { return symbol_; }
    int64_t getQuantity() const { return quantity_; }
    double getAveragePrice() const { return averagePrice_; }
    double getRealizedPnL() const { return realizedPnL_; }
    
    /**
     * Calculate unrealized P&L based on current market price
     */
    double getUnrealizedPnL(double currentPrice) const {
        if (quantity_ == 0) return 0.0;
        return quantity_ * (currentPrice - averagePrice_);
    }
    
    /**
     * Calculate total P&L (realized + unrealized)
     */
    double getTotalPnL(double currentPrice) const {
        return realizedPnL_ + getUnrealizedPnL(currentPrice);
    }
    
    bool isFlat() const { return quantity_ == 0; }
    bool isLong() const { return quantity_ > 0; }
    bool isShort() const { return quantity_ < 0; }

    /**
     * Apply a fill to the position.
     * Handles both opening and closing trades with P&L calculation.
     */
    void applyFill(Side side, int64_t fillQuantity, double fillPrice) {
        int64_t signedQuantity = (side == Side::BUY) ? fillQuantity : -fillQuantity;
        
        // Check if this trade closes or reduces existing position
        bool sameDirection = (quantity_ >= 0 && signedQuantity > 0) || 
                            (quantity_ < 0 && signedQuantity < 0);
        
        if (!sameDirection && quantity_ != 0) {
            // Closing or reducing position - calculate realized P&L
            int64_t closingQuantity = std::min(std::abs(signedQuantity), std::abs(quantity_));
            
            if (quantity_ > 0) {
                // Closing long position
                realizedPnL_ += closingQuantity * (fillPrice - averagePrice_);
            } else {
                // Closing short position
                realizedPnL_ += closingQuantity * (averagePrice_ - fillPrice);
            }
            
            quantity_ += signedQuantity;
            
            // If position flips direction, new average price is the fill price
            if ((quantity_ > 0 && signedQuantity > 0) || (quantity_ < 0 && signedQuantity < 0)) {
                averagePrice_ = fillPrice;
            }
        } else {
            // Opening new position or adding to existing
            if (quantity_ == 0) {
                averagePrice_ = fillPrice;
            } else {
                // Calculate new average price
                int64_t totalQuantity = quantity_ + signedQuantity;
                averagePrice_ = (averagePrice_ * quantity_ + fillPrice * signedQuantity) / totalQuantity;
            }
            quantity_ += signedQuantity;
        }
    }

    /**
     * Reset position to flat (useful for testing)
     */
    void reset() {
        quantity_ = 0;
        averagePrice_ = 0.0;
        realizedPnL_ = 0.0;
    }

private:
    std::string symbol_;
    int64_t quantity_;        // Positive = long, negative = short
    double averagePrice_;     // Average entry price
    double realizedPnL_;      // Realized profit/loss from closed trades
};

} // namespace engine
