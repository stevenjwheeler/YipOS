#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>

typedef void CURL;

namespace YipOS {

struct StockQuote {
    std::string symbol;           // user-facing: "DOGE", "BTC", "AAPL"
    float current_price = 0;
    float prev_close = 0;         // previous close for % change
    std::vector<float> history;   // downsampled close prices for graph
    std::string time_label;       // "1DY", "1WK", "1MO", "6MO", "1YR"
    int64_t last_fetch = 0;
};

class StockClient {
public:
    StockClient();
    ~StockClient();

    bool FetchQuote(const std::string& symbol, const std::string& window);
    const StockQuote* GetQuote(const std::string& symbol) const;
    const std::vector<StockQuote>& GetQuotes() const { return quotes_; }

    // Symbols to track
    void SetSymbols(const std::vector<std::string>& symbols) { symbols_ = symbols; }
    const std::vector<std::string>& GetSymbols() const { return symbols_; }

    // Fetch all tracked symbols for the given window
    void FetchAll(const std::string& window);

    static constexpr int GRAPH_WIDTH = 33;

private:
    std::string BuildURL(const std::string& symbol, const std::string& window) const;
    std::string ToYahooSymbol(const std::string& symbol) const;
    std::string RangeForWindow(const std::string& window) const;
    std::string IntervalForWindow(const std::string& window) const;
    bool ParseResponse(const std::string& json, StockQuote& quote);
    void Downsample(std::vector<float>& data, int target_size);

    // Known crypto symbols (auto-suffixed with -USD)
    static bool IsCrypto(const std::string& symbol);

    CURL* curl_ = nullptr;
    std::vector<StockQuote> quotes_;
    std::vector<std::string> symbols_;
    std::unordered_map<std::string, size_t> symbol_index_;  // symbol → index in quotes_
};

} // namespace YipOS
