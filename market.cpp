// Project Identifier: 0E04A31E0D60C01986ACB20081C9D8722A1899B6

#include <iostream>
#include <algorithm>
#include <queue>
#include <cassert>
#include <getopt.h>
#include <cstdint>
#include <sstream>
#include "P2random.h"

using namespace std;


class market    {
    public:
        market(): CURRENT_TIMESTAMP(0), isV(false), isT(false), isI(false), isM(false){}
        
        void getMode(int argc, char * argv[])   {
            // These are used with getopt_long()
            opterr = false; // Let us handle all error output for command line options
            int choice;
            int index = 0;
            option long_options[] = {
                {"verbose", no_argument, nullptr, 'v'},
                {"median", no_argument, nullptr, 'm'},
                {"trader_info", no_argument, nullptr, 'i'},
                {"time_travelers", no_argument, nullptr, 't'},
                { nullptr, 0, nullptr, '\0' },
            };  // long_options[]

            while ((choice = getopt_long(argc, argv, "vmit", long_options, &index)) != -1) {
                switch(choice)  {
                    case 'v':
                        isV = true;;
                        break;
                    case 'm':
                        isM = true;
                        break;
                    case 'i':
                        isI = true;
                        break;
                    case 't':
                        isT = true;
                        break;
                    default:
                        cerr << "Error: invalid option\n";
                        exit(1);
                }
            }
        }


        void handleData()   {
            readHeader();

            if (isTL)   {
                processOrders(cin);
            }
            else    {
                stringstream ss;
                //Read PR header and handle
                string junk;
                uint32_t seed;
                uint32_t numOrders;
                uint32_t rate;


                cin >> junk >> seed;
                cin >> junk >> numOrders;
                cin >> junk >> rate;


                P2random::PR_init(ss, seed, numTraders, numStocks, numOrders, rate);
                processOrders(ss);
            }
        }


    private:
        uint32_t CURRENT_TIMESTAMP;
        bool isV;
        bool isT;
        bool isI;
        bool isM;
        bool isTL;
        uint32_t numTraders;
        uint32_t numStocks;

        struct order{
            order(): timestamp(0), line(0), traderID(0), isBuy(true), stockNum(0), 
                    pricePerShare(0), amountOfStock(0){}
            order(uint32_t ts, uint32_t l, uint32_t tID, bool isB, uint32_t sNum, uint32_t pps, uint32_t aos):
            timestamp(ts), line(l), traderID(tID), isBuy(isB), stockNum(sNum), 
            pricePerShare(pps), amountOfStock(aos){}

            uint32_t timestamp;
            uint32_t line;
            uint32_t traderID;
            bool isBuy;
            uint32_t stockNum; 
            uint32_t pricePerShare; 
            uint32_t amountOfStock;
        };
        struct trade    {
            trade(): buyerID(0), sellerID(0), stockNum(0), amountOfStock(0), salePrice(0){}
            trade(uint32_t bID, uint32_t sID, uint32_t sNum, uint32_t amt, uint32_t sP): 
            buyerID(bID), sellerID(sID), stockNum(sNum), amountOfStock(amt), salePrice(sP){}

            uint32_t buyerID;
            uint32_t sellerID;
            uint32_t stockNum;
            uint32_t amountOfStock;
            uint32_t salePrice;

        };
        
        struct trackTrades  {
            trackTrades(): numBought(0), numSold(0), net(0){}
            uint32_t numBought;
            uint32_t numSold;
            int net;
        };
        vector<trackTrades> allTraderInfo;


        //For priority queue max heap
        struct orderComparatorA  {
            bool operator()(const order& a, const order& b) {
                if (a.pricePerShare != b.pricePerShare) {
                    return a.pricePerShare < b.pricePerShare;
                }
                //Integrate linenum
                return a.line > b.line;
            }
        };
        //for priority queue min heap
        struct orderComparatorB  {
            bool operator()(const order& a, const order& b) {
                if (a.pricePerShare != b.pricePerShare) {
                    return a.pricePerShare > b.pricePerShare;
                }
                //Integrate linenum
                return a.line > b.line;
            }
        };

        //for priority queue of trades max heap
        struct tradeComparatorA {
            bool operator()(const trade& a, const trade& b) {
                return a.salePrice < b.salePrice;
            }
        };

        //for priority queue of trades min heap
        struct tradeComparatorB {
            bool operator()(const trade& a, const trade& b) {
                return a.salePrice > b.salePrice;
            }
        };

        struct stock    {
            //sort by higher first
            priority_queue<order, vector<order>, orderComparatorA> buyOrders;

            //sort by smallest first
            priority_queue<order, vector<order>, orderComparatorB> sellOrders;

            struct allTrades {
                priority_queue<trade, vector<trade>, tradeComparatorA> bottomHalf;
                priority_queue<trade, vector<trade>, tradeComparatorB> topHalf;

                void addTrade(trade& t)  {
                    if (bottomHalf.empty() || bottomHalf.top().salePrice > t.salePrice)   {
                        bottomHalf.push(t);
                    }
                    else  {
                        topHalf.push(t);
                    }
                    if (bottomHalf.size() > topHalf.size() + 1) {
                        topHalf.push(bottomHalf.top());
                        bottomHalf.pop();
                    }
                    else if (topHalf.size() > bottomHalf.size())    {
                        bottomHalf.push(topHalf.top());
                        topHalf.pop();
                    }
                }
                uint32_t getMedian()  const {
                    if (bottomHalf.size() != topHalf.size())    {
                        return bottomHalf.top().salePrice;
                    }
                    else    {
                        return (bottomHalf.top().salePrice + topHalf.top().salePrice)/2;
                    }
                }
                uint32_t getTradeCount()    const {
                    return static_cast<uint32_t>(bottomHalf.size() + topHalf.size());
                }
            };

            struct timeTraveler {
                timeTraveler(): profit(0), maxSell(UINT32_MAX, 0), minBuy(UINT32_MAX, 0), newBuyP(UINT32_MAX, 0){}

                uint32_t profit;
                //max sell order for highest profit
                pair<uint32_t, uint32_t> maxSell;
                //min buy order for highest profit
                pair<uint32_t, uint32_t> minBuy;
                
                //current min buy.  May not maximize profit but keep track
                pair<uint32_t, uint32_t> newBuyP;

                void handleOrder(order& o)   {
                    if (!o.isBuy)    {
                        if (newBuyP.first == UINT32_MAX || o.pricePerShare < newBuyP.first) {
                            newBuyP.first = o.pricePerShare;
                            newBuyP.second = o.timestamp;
                        }
                    }
                    else    {
                        if (o.pricePerShare > newBuyP.first 
                        && o.pricePerShare - newBuyP.first > profit)    {
                            profit = o.pricePerShare - newBuyP.first;
                            maxSell = {o.pricePerShare, o.timestamp};
                            minBuy = {newBuyP.first, newBuyP.second};
                        }
                    }
                };
                bool canMakeProfit()   const {
                    return profit > 0;
                };
            };

            allTrades hm;
            timeTraveler tt;
        };

        
        
        //vector of stocks, each stock represented
        vector<stock> allStocks;


        void readHeader()   {
            //Read in Header
            string junk;
            string commentLine;
            string modeLine;
            getline(cin, commentLine);
            
            cin >> junk >> modeLine;
            cin >> junk >> numTraders;
            cin >> junk >> numStocks;


            assert(modeLine[0] == 'P' || modeLine[0] == 'T');
            if (modeLine == "TL") {
                isTL = true;
            }
            else    {
                isTL = false;
            }   
            
            
            allStocks.resize(numStocks, stock());
            allTraderInfo.resize(numTraders, trackTrades());
        }

        void processOrders(istream &inputStream) {
            //Step 1 Spec
            printStartupOutput();

            char junk;
            int ts;
            string type;
            bool isB;
            int trID;
            int stNum;
            int pricePS;
            int amtOfS;
            uint32_t line = 0;

            while(inputStream >> ts >> type >> junk >> trID >> junk
                >> stNum >> junk  >> pricePS >> junk >> amtOfS)    {
                    if (ts < 0) {
                        cerr << "Negative timestamp invalid\n";
                        exit(1);
                    }
                    if (ts > (int)CURRENT_TIMESTAMP)    {
                        if (isM) {
                            medianOutput();
                        }
                        CURRENT_TIMESTAMP = static_cast<uint32_t>(ts);
                    }
                    if (ts < (int)CURRENT_TIMESTAMP) {
                        cerr << "Invalid Timestamp\n";
                        exit(1);
                    }


                    if (type == "BUY") {
                        isB = true;
                    }
                    else {
                        isB = false;
                    }


                    if (trID >= (int)numTraders || trID < 0)  {
                        cerr << "Invalid trader ID\n";
                        exit(1);
                    }

                    if (stNum >= (int)numStocks || stNum < 0) {
                        cerr << "Invalid stock ID\n";
                        exit(1);
                    }

                    if (pricePS <= 0 || amtOfS <= 0) {
                        cerr << "Invalid Price per Share or amount of stock\n";
                        exit(1);
                    }

                    uint32_t t = static_cast<uint32_t>(ts);
                    uint32_t tID = static_cast<uint32_t>(trID);
                    uint32_t sN = static_cast<uint32_t>(stNum);
                    uint32_t pps = static_cast<uint32_t>(pricePS);
                    uint32_t aos = static_cast<uint32_t>(amtOfS);
                    order thisOrder(t, line, tID, isB, sN, pps, aos);
                    makeAllMatches(thisOrder);
                    ++line;


            }
            if (isM)    {
                medianOutput();
            }
            summaryOutput();
            if (isI)    {
                outputTraderInfo();
            }
            if (isT)    {
                outputTimeTraveler();
            }

        }

        
        void printStartupOutput() const   {
            cout << "Processing orders...\n";
        }

        void medianOutput() {
            for (uint32_t i = 0; i < allStocks.size(); ++i) {
                if (allStocks[i].hm.getTradeCount() > 0) {
                    cout << "Median match price of Stock " << i << " at time " << CURRENT_TIMESTAMP << " is $"
                    << allStocks[i].hm.getMedian() << "\n";
                }
            }
        }

        void verboseOutput(order buyer, order seller, uint32_t amount, uint32_t ppc)    {
            cout << "Trader " << buyer.traderID << " purchased " << amount << " shares of Stock "
            << buyer.stockNum << " from Trader " << seller.traderID << " for $" << ppc
            << "/share\n";
        };
        
        void summaryOutput()  const {
            uint32_t total = 0;
            for (auto& stock : allStocks)   {
                total += stock.hm.getTradeCount();
            }
            cout << "---End of Day---\nTrades Completed: " << total << "\n";
        }

        void outputTraderInfo() const {
            cout << "---Trader Info---\n";
            for (uint32_t i = 0; i < numTraders; ++i)   {
                cout << "Trader " << i << " bought " << allTraderInfo[i].numBought << " and sold " 
                << allTraderInfo[i].numSold << " for a net transfer of $" << allTraderInfo[i].net << "\n";
            }
        }

        void outputTimeTraveler()   {
            cout << "---Time Travelers---\n";
            for (uint32_t i = 0; i < allStocks.size(); ++i)    {
                if (allStocks[i].tt.canMakeProfit())  {
                    cout << "A time traveler would buy Stock "
                    << i << " at time " << allStocks[i].tt.minBuy.second << " for $"
                    << allStocks[i].tt.minBuy.first << " and sell it at time "
                    <<allStocks[i].tt.maxSell.second << " for $" 
                    << allStocks[i].tt.maxSell.first << "\n";
                }
                else    {
                    cout << "A time traveler could not make a profit on Stock " << i << "\n";
                }
            }
        }

        void makeAllMatches(order& o)    {
            allStocks[o.stockNum].tt.handleOrder(o);
            if (o.isBuy)    {
                makeAllMatchesB(o);
            }
            else    {
                makeAllMatchesS(o);
            }
        } 
        void makeAllMatchesB(order& o)  {
            stock& stock = allStocks[o.stockNum];
            while(o.amountOfStock > 0 && !stock.sellOrders.empty())   {
                order seller = stock.sellOrders.top();
                if (o.pricePerShare < seller.pricePerShare)    {
                    break;
                }
                uint32_t amount = min(o.amountOfStock, seller.amountOfStock);
                trade newTrade(o.traderID, seller.traderID, o.stockNum, amount, seller.pricePerShare);
                allTraderInfo[o.traderID].numBought += newTrade.amountOfStock;
                allTraderInfo[o.traderID].net -= newTrade.amountOfStock * newTrade.salePrice;
                allTraderInfo[seller.traderID].numSold += newTrade.amountOfStock;
                allTraderInfo[seller.traderID].net += newTrade.amountOfStock * newTrade.salePrice;

                stock.hm.addTrade(newTrade);

                if (isV)  {
                    verboseOutput(o, seller, amount, seller.pricePerShare);
                }

                stock.sellOrders.pop();
                if (amount != seller.amountOfStock) {
                    seller.amountOfStock -= amount;
                    stock.sellOrders.push(seller);
                }
                o.amountOfStock -= amount;
                
            }
            if (o.amountOfStock > 0)    {
                stock.buyOrders.push(o);
            }
        }
        
        void makeAllMatchesS(order& o)   {
            stock& stock = allStocks[o.stockNum];
            while(o.amountOfStock > 0 && !stock.buyOrders.empty())   {

                order buyer = stock.buyOrders.top();
                if (o.pricePerShare > buyer.pricePerShare)   {
                    break;
                }
                uint32_t amount = min(o.amountOfStock, buyer.amountOfStock);
                trade newTrade(buyer.traderID, o.traderID, o.stockNum, amount, buyer.pricePerShare);
                allTraderInfo[o.traderID].numSold += newTrade.amountOfStock;
                allTraderInfo[o.traderID].net += newTrade.amountOfStock * newTrade.salePrice;
                allTraderInfo[buyer.traderID].numBought += newTrade.amountOfStock;
                allTraderInfo[buyer.traderID].net -= newTrade.amountOfStock * newTrade.salePrice;
                
                stock.hm.addTrade(newTrade);

                if (isV)  {
                    verboseOutput(buyer, o, amount, buyer.pricePerShare);
                }

                stock.buyOrders.pop();
                if (amount != buyer.amountOfStock) {
                    buyer.amountOfStock -= amount;
                    allStocks[o.stockNum].buyOrders.push(buyer);
                }
                o.amountOfStock -= amount;
            }
            if (o.amountOfStock > 0)    {
                stock.sellOrders.push(o);
            }
        }

    

    
};

int main(int argc, char ** argv)  {
    market m;
    m.getMode(argc, argv);
    m.handleData();
}