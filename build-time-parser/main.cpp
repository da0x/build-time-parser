    //  Copyright 2016 Daher Alfawares
    //
    //  Licensed under the Apache License, Version 2.0 (the "License");
    //  you may not use this file except in compliance with the License.
    //  You may obtain a copy of the License at
    //  
    //      http://www.apache.org/licenses/LICENSE-2.0
    //
    //  Unless required by applicable law or agreed to in writing, software
    //  distributed under the License is distributed on an "AS IS" BASIS,
    //  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    //  See the License for the specific language governing permissions and
    //  limitations under the License.


#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include "core_tree.hpp"

class Interval {
    std::time_t seconds;
    
public:
    Interval(std::time_t seconds): seconds(seconds) {}
    
    std::time_t hours(){
        return seconds/(60.0*60.0);
    }
    
    double business_days(){
        double days = seconds/(60.0*60.0*24.0);
        double days_per_week = 7.0;
        double business_days_per_week = 5.0;
        double business_days = days * business_days_per_week / days_per_week;
        return business_days;
    }
    
    std::time_t minutes_after_hours(){
        seconds -= floor(hours()) * 60.0 * 60.0;
        return seconds/60.0;
    }
    
};


class Time {
    std::time_t seconds;
    
public:
    Time(std::time_t seconds): seconds(seconds) {}
    
    std::time_t days_since_epoch(){
        return std::floor(Interval(seconds).hours()/24.0);
    }
    
    std::string string(){
        std::stringstream stream;
        std::tm tm = *std::localtime(&seconds);
        std::cout.imbue(std::locale());
        stream << std::put_time(&tm, "%Y/%m/%d");
        return stream.str();
    }
};


class Average {
    double average;
    double count;
    
public:
    Average(): average(0),count(0){}
    
    Average& operator += (double add){
        double total = average * count++;
        average = (total + add) / count;
        return *this;
    }
    
    operator double(){
        return average;
    }
    
    double iterations(){
        return this->count;
    }
    
    double total(){
        return average * count;
    }
};

namespace parse {
    
    struct format {
        std::string build_file;
        std::string start_date;
        std::string end_date;
        std::string actual;
        std::string build_count;
        std::string build_average_time;
        std::string build_total_time;
        std::string builds_per_day;
        std::string time_lost_per_day;
    };
    
    
    std::string next_line(std::ifstream &stream){
        const size_t bufferSize = 1024;
        char buffer[bufferSize] = {0};
        
        stream.getline(buffer, bufferSize);
        return std::string(buffer);
    }
    
    std::time_t time_from_line(std::string line_text){
        std::istringstream str(line_text);
        
        std::string text;
        std::string time;
        
        str >> text >> time;
        
        std::time_t interval = std::stoi(time);
        return interval;
    }

    format file(std::string filename, bool verbose){
        parse::format format;
        
        std::ifstream logfile(filename.c_str());
        if(!logfile){
            std::cerr << "Can't open file " << filename << std::endl;
            
            format.build_file = "X " + filename;
            return format;
        }
        
        std::string startedText   = parse::next_line(logfile);
        std::string endedText     = parse::next_line(logfile);
        std::string endedConstant = "FINISHED";
        std::time_t firstDate     = parse::time_from_line(startedText);
        std::time_t lastDate      = parse::time_from_line(startedText);
        std::time_t currentDay    = Time(firstDate).days_since_epoch();
        std::time_t days          = 0;
        
        Average average;
        
        while( logfile ){
            
            while( logfile && !std::equal( endedConstant.begin(), endedConstant.end(), endedText.begin() ) ){
                startedText = endedText;
                endedText = parse::next_line(logfile);
            }
            
            if( startedText.length() == 0 || endedText.length() == 0 ){
                break;
            }
            
            if (verbose){
                std::cout << "Found combo: " << std::endl;
                std::cout << " --- " << startedText << std::endl;
                std::cout << " --- " << endedText << std::endl;
            }
            
            std::time_t start = time_from_line(startedText);
            std::time_t end   = time_from_line(endedText);
            
            if (verbose){
                std::cout << "Interval combo: " << std::endl;
                std::cout << " --- " << start << std::endl;
                std::cout << " --- " << end << std::endl;
                std::cout << " --- Seconds = " << end - start << std::endl;
            }
            
            if (start != end) {
                average += static_cast<double>( end-start );
            }
            
                // get last
            lastDate = parse::time_from_line(startedText);
            auto days_since_epoch = Time(lastDate).days_since_epoch();
            if( days_since_epoch != currentDay ){
                days ++;
                currentDay = days_since_epoch;
            }
            
                // next
            startedText = next_line(logfile);
            endedText = next_line(logfile);
        }
        
        
        {
        std::stringstream str;
        str << filename;
        format.build_file = str.str();
        }
        {
        std::stringstream str;
        str << Time(firstDate).string();
        format.start_date = str.str();
        }
        {
        std::stringstream str;
        str << Time(lastDate).string();
        format.end_date = str.str();
        }
        {
        std::stringstream str;
        str << days << " business days";
        format.actual = str.str();
        }
        {
        std::stringstream str;
        str << average.iterations();
        format.build_count = str.str();
        }
        {
        std::stringstream str;
        str << average << " s";
        format.build_average_time = str.str();
        }
        {
        std::stringstream str;
        str << std::floor(Interval(average.total()).hours()) << " hrs "
        << Interval(average.total()).minutes_after_hours() << " s";
        format.build_total_time = str.str();
        }
        {
        std::stringstream str;
        str << average.iterations()/days << " builds";
        format.builds_per_day = str.str();
        }
        {
        std::stringstream str;
        str << std::floor(Interval(average.total()/days).hours()) << " hrs "
        << Interval(average.total()/days).minutes_after_hours() << " m";
        format.time_lost_per_day = str.str();
        }
        
        return format;
    }
}

namespace arg {
    bool is_option(std::string arg){
        if( arg.length() ){
            if( arg[0] == '-')
                return true;
        }
        return false;
    }

    bool is_verbose(int argc, const char * argv[]){
        for( int i =1; i< argc; i++ ){
            std::string arg = argv[i];
            
            if( arg == "-verbose"){
                return true;
            }
        }
        
        return false;
    }
}

int main(int argc, const char * argv[]) {
        // insert code here...
    bool verbose = arg::is_verbose(argc,argv);
    
    if (argc < 2){
        std::cout << "Usage: build-time-parser [-verbose] <filename> <filename> <...>" << std::endl;
        std::cout << std::endl;
        std::cout << "Example: build-time-parser joh.log robert.log erika.log" << std::endl;
        std::cout << "Example: build-time-parser -verbose joh.log robert.log erika.log" << std::endl;
        return 1;
    }
    
    ascii::table myTable("Build time statistics");
    myTable
        ("build file")
        ("start date")
        ("end date")
        ("actual")
        ("build count")
        ("average time")
        ("total time")
        ("builds per day")
        ("time lost per day")++;
    
    for( int i = 1; i< argc; i++ ){
        std::string filename = argv[i];
        
        if(arg::is_option(filename)){
            continue;
        }
        
        parse::format format = parse::file(filename,verbose);
        
        myTable
            (format.build_file)
            (format.start_date)
            (format.end_date)
            (format.actual)
            (format.build_count)
            (format.build_average_time)
            (format.build_total_time)
            (format.builds_per_day)
            (format.time_lost_per_day)++;
    }
    
    std::cout << myTable << std::endl;
    return 0;
}
