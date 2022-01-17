#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <math.h>

using namespace std;

#define MAX_VIEW_LIST 10

#define DBL_QUOTA "\""

ofstream outLoopFile, outLogFile;

struct City_Info
{
    string cityName;
    float rat;
    float lng;
};

struct Dlv_List_Info
{
    int cnt;
    int orgIndex;
    int tgtIndex;
    string name, shipper, trailer;
};

struct Dlv_Loop
{
    int cum_dist;  // cumulative distance
    int prev_city; // previous city index
    bool btsk;     // task complete state
    string name, shipper, trailer;
};

vector<City_Info> all_city_list;
vector<City_Info> city_list;
vector<Dlv_List_Info> dlv_list;
vector<vector<int>> dist;
vector<vector<Dlv_Loop>> opt_loop;

int startp, endp, deadhead, max_dist;
int city_cnt, total_job_cnt, list_cnt;

void split(const std::string &s, char delim, std::vector<std::string> &elems)
{
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim))
    {
        elems.push_back(item);
    }
}

void readCityInformation()
{
    std::ifstream infile("./uscities.txt");
    std::string line;

    all_city_list.clear();
    while (std::getline(infile, line))
    {
        vector<string> row_values;
        split(line, '\t', row_values);

        City_Info ct_info;
        ct_info.cityName = row_values[0] + ", " + row_values[1];
        ct_info.rat = std::stof(row_values[2]);
        ct_info.lng = std::stof(row_values[3]);

        all_city_list.push_back(ct_info);
    }
    //     for (auto v: all_city_list)
    //        cout << v.rat << ','<<v.cityName<<endl ;
    infile.close();
}

int findCityIndex(string cityName)
{
    int i = 0;
    for (auto v : city_list)
    {
        if (v.cityName == cityName)
        {
            return i;
        }

        i++;
    }

    i = 0;
    for (auto v : all_city_list)
    {
        if (v.cityName == cityName)
            break;
        i++;
    }

    if (i == all_city_list.size())
        return -1;

    city_list.push_back(all_city_list[i]);

    return city_list.size() - 1;
}
void readDlvList()
{
    std::ifstream infile("./load_list.txt");
    std::string line;
    int i = 0;

    city_list.clear();
    dlv_list.clear();

    total_job_cnt = 0;
    while (std::getline(infile, line))
    {
        i++;
        if (i < 4)
            continue;

        vector<string> row_values;
        split(line, '\t', row_values);
        //search_CityIndex();
        Dlv_List_Info dlv_info;
        dlv_info.name = row_values[0];
        dlv_info.shipper = row_values[1];
        dlv_info.cnt = std::stod(row_values[2]);
        int row_cnt = row_values.size();
        total_job_cnt += dlv_info.cnt;

        if (row_cnt > 8)
        {

            if (row_values[7].length() > 8 && row_values[8].length() > 8)
            {
                row_values[7] = row_values[7].substr(0, row_values[7].length() - 5);
                row_values[8] = row_values[8].substr(0, row_values[8].length() - 5);

                dlv_info.orgIndex = findCityIndex(row_values[7]);
                dlv_info.tgtIndex = findCityIndex(row_values[8]);

                if (dlv_info.orgIndex < 0)
                    outLogFile << "Database has no data of  : " << row_values[7] << endl;
                if (dlv_info.tgtIndex < 0)
                    outLogFile << "Database has no data of  : " << row_values[8] << endl;
                if (row_cnt > 11)
                    dlv_info.trailer = row_values[11];
                else
                    dlv_info.trailer = "";

                if (dlv_info.orgIndex >= 0 && dlv_info.tgtIndex >= 0)
                    dlv_list.push_back(dlv_info);
            }
        }
        else
        {
            outLogFile << "----------------------------------------------" << endl;
            outLogFile << "Wrong data -> : " << line << endl;
            outLogFile << "----------------------------------------------" << endl;
        }
    }
    infile.close();

    cout << "Database loading complete!\r\n\r\n";
    list_cnt = dlv_list.size();
}

bool readInputParams()
{
    std::ifstream infile("./input_params.txt");
    std::string line;

    std::getline(infile, line);
    startp = findCityIndex(line);
    if (startp < 0)
        outLogFile << "Database has no data of  Orign City: " << line << endl;

    std::getline(infile, line);

    endp = findCityIndex(line);
    if (endp < 0)
        outLogFile << "Database has no data of  Destination City: " << line << endl;

    std::getline(infile, line);
    max_dist = std::stof(line);

    std::getline(infile, line);
    deadhead = std::stof(line);
    ;

    infile.close();

    if (startp < 0 || endp < 0)
    {
        outLogFile << "Wrong Input Parameters!\r\nCheck your input_params.txt's content\r\n";
        cout << "Wrong Input Parameters!\r\nCheck your input_params.txt's content\r\n";
        return false;
    }

    city_cnt = city_list.size();
    cout << "\r\n\r\n------------------Input Parameters--------------------------\r\n\r\n";
    cout << "=>                Origin City is        :   " << city_list[startp].cityName << endl;
    cout << "=>                Destination City is   :   " << city_list[endp].cityName << endl;
    cout << "=>     Available  Maximum Distance is   :   " << max_dist << " miles" << endl;
    cout << "=>     Available  Maximum Deadhead is   :   " << deadhead << " miles" << endl;
    cout << "\r\n------------------------------------------------------------\r\n\r\n";

    cout << "Input parameters loading complete!\r\n\r\n";
    return true;
}

double toRad(double degree)
{
    return degree / 180 * 3.141592;
}

int calculateDistance(double lat1, double long1, double lat2, double long2)
{
    double dist;
    dist = sin(toRad(lat1)) * sin(toRad(lat2)) + cos(toRad(lat1)) * cos(toRad(lat2)) * cos(toRad(long1 - long2));
    dist = acos(dist);

    dist = 6371 * dist * 0.621371;
    return dist;
}

void makeDistanceMatrix()
{
    dist.clear();
    dist.resize(city_cnt, vector<int>(city_cnt));
    for (int i = 0; i < city_list.size(); i++)
    {
        dist[i][i] = 0;
        for (int j = i + 1; j < city_list.size(); j++)
        {
            dist[i][j] = calculateDistance(city_list[i].rat, city_list[i].lng, city_list[j].rat, city_list[j].lng);
            dist[j][i] = dist[i][j];
        }
    }
}

int DoYouHaveJob(int sp, int ep, string &name, string &shipper, string &trailer)
{
    for (int i = 0; i < list_cnt; i++)
    {
        if (dlv_list[i].orgIndex == sp && dlv_list[i].tgtIndex == ep && dlv_list[i].cnt > 0)
        {
            name = dlv_list[i].name;
            shipper = dlv_list[i].shipper;
            trailer = dlv_list[i].trailer;
            return i;
        }
    }
    name = "";
    shipper = "";
    trailer = "";

    return -1;
}
void Go_deliver(int cur_city, int done_cnt)
{
    int i, tgt_city, accum_dist;

    for (tgt_city = 0; tgt_city < city_cnt; tgt_city++)
    {
        if (tgt_city == cur_city)
            continue;

        accum_dist = opt_loop[cur_city][done_cnt].cum_dist + dist[cur_city][tgt_city];
        if (accum_dist >= max_dist)
            continue;

        string cur_name, cur_shipper, cur_trailer;

        i = DoYouHaveJob(cur_city, tgt_city, cur_name, cur_shipper, cur_trailer);
        if (i >= 0) // in the case existing in the list.
        {
            if (accum_dist >= opt_loop[tgt_city][done_cnt + 1].cum_dist)
                continue;
            opt_loop[tgt_city][done_cnt + 1].cum_dist = accum_dist;
            opt_loop[tgt_city][done_cnt + 1].btsk = true;
            opt_loop[tgt_city][done_cnt + 1].prev_city = cur_city;
            opt_loop[tgt_city][done_cnt + 1].name = cur_name;
            opt_loop[tgt_city][done_cnt + 1].shipper = cur_shipper;
            opt_loop[tgt_city][done_cnt + 1].trailer = cur_trailer;

            dlv_list[i].cnt--; // one task complete
            Go_deliver(tgt_city, done_cnt + 1);
            dlv_list[i].cnt++; // one task complete
            continue;
        }

        if (dist[cur_city][tgt_city] > deadhead)
            continue;

        if (accum_dist >= opt_loop[tgt_city][done_cnt].cum_dist)
            continue;

        opt_loop[tgt_city][done_cnt].cum_dist = accum_dist;
        opt_loop[tgt_city][done_cnt].btsk = false;
        opt_loop[tgt_city][done_cnt].prev_city = cur_city;
        opt_loop[tgt_city][done_cnt].name = "";
        opt_loop[tgt_city][done_cnt].shipper = "";
        opt_loop[tgt_city][done_cnt].trailer = "";

        Go_deliver(tgt_city, done_cnt);
    }
}

#define _INT_MAX 700000
void searchOptimizationLoop()
{
    opt_loop.clear();
    opt_loop.resize(city_cnt, vector<Dlv_Loop>(total_job_cnt));

    for (int i = 0; i < city_cnt; i++)
    {
        for (int j = 0; j < total_job_cnt; j++)
        {
            opt_loop[i][j].cum_dist = _INT_MAX;
            opt_loop[i][j].btsk = false;
            opt_loop[i][j].prev_city = -1;
            opt_loop[i][j].name = "";
            opt_loop[i][j].shipper = "";
            opt_loop[i][j].trailer = "";
        }
    }
    opt_loop[startp][0].cum_dist = 0;
    Go_deliver(startp, 0);
}

void Print_Loop(int cur_city, int done_cnt)
{
    outLoopFile << "\"Number Carried\",\"Total distance [ miles ]\",\"Total deadhead [ miles ]\",\"Ratio [ % ]\"" << endl;

    int total = opt_loop[cur_city][done_cnt].cum_dist, deadhead = 0, total_done = done_cnt;

    int prev_city;
    int flag, delta;
    string mss_status, name, shipper, trailer;

    vector<Dlv_List_Info> result_list;
    Dlv_List_Info temp_row;

    result_list.clear();

    while (1)
    {
        prev_city = opt_loop[cur_city][done_cnt].prev_city;

        if (prev_city < 0)
            break;

        temp_row.orgIndex = prev_city;
        temp_row.tgtIndex = cur_city;
        temp_row.name = opt_loop[cur_city][done_cnt].name;
        temp_row.shipper = opt_loop[cur_city][done_cnt].shipper;
        temp_row.trailer = opt_loop[cur_city][done_cnt].trailer;
        temp_row.cnt = done_cnt;

        

        delta = 0;
        string qq;
        qq = "NO  ";
        if (opt_loop[cur_city][done_cnt].btsk)
        {
            delta = 1;
            qq = "OK  ";
        }

        if (delta == 0)
        {
            deadhead += opt_loop[cur_city][done_cnt].cum_dist - opt_loop[prev_city][done_cnt].cum_dist;
        }
        result_list.push_back(temp_row);

        //cout << qq << temp_row.name << "  " << temp_row.shipper << "  " << city_list[prev_city].cityName << " " << city_list[cur_city].cityName << "  " << temp_row.trailer << "\"" << endl;
        //system("pause");
        done_cnt -= delta;
        cur_city = prev_city;
    }
    //return;

    outLoopFile << total_done << ", " << total << ", " << deadhead << ", " << (int)(deadhead / (float)total * 100.0+0.5) << endl
                << endl;

    outLoopFile << "\"Name\",\"Shipper\",\"Orign\",\"Destination\",\"Status\",\"Trailer\",\"Distance[miles]\"" << endl;

    while (1)
    {
        temp_row = result_list.back();
        result_list.pop_back();
        prev_city = temp_row.orgIndex;
        cur_city = temp_row.tgtIndex;
        done_cnt = temp_row.cnt;
        name = temp_row.name;
        shipper = temp_row.shipper;
        trailer = temp_row.trailer;

        mss_status = "\"unloaded\",";
        delta = 0;
        if (opt_loop[cur_city][done_cnt].btsk)
        {
            mss_status = "\"loaded\",";
            delta = 1;
        }
        outLoopFile << "\"" << name << "\",\"" << shipper << "\",\"" << city_list[prev_city].cityName << "\",\"" << city_list[cur_city].cityName << "\"," << mss_status <<  trailer << ",\"" << opt_loop[cur_city][done_cnt].cum_dist - opt_loop[prev_city][done_cnt - delta].cum_dist<< "\"" << endl;

        if (result_list.size() == 0)
            break;
    }
    outLoopFile << endl
                << endl
                << endl;
}

void outputResult()
{
    outLoopFile.open("optLoop.csv");
    vector<int> idx_series;
    idx_series.clear();
    int i;
    for (i = 0; i < total_job_cnt; i++)
    {
        if (opt_loop[endp][i].cum_dist != _INT_MAX)
            idx_series.push_back(i);
    }
    if (idx_series.size() == 0)
    {
        outLoopFile << "The Loop does not exist!\r\n";
        return;
    }
    int temp = 0, idx;
    while (idx_series.size() != 0)
    {

        idx = idx_series.back();
        idx_series.pop_back();

        Print_Loop(endp, idx);

        temp++;
        if (temp > MAX_VIEW_LIST)
            break;
    }

    outLoopFile.close();
}

int main()
{
    outLogFile.open("out.log");

    readCityInformation();
    readDlvList();
    if (!readInputParams())
        return -1;

    makeDistanceMatrix();
    searchOptimizationLoop();
    outputResult();

    cout << "Calculation complete!\r\n\r\n";
    cout << "Please view files - optLoop.csv and out.log!\r\n\r\n";
    outLogFile.close();
    system("pause");
    return 0;
}
