#include <stdio.h>
#include <stdlib.h>
#include "edge_mt_stats.h"

int main (int argc, char *argv[])
{
    mt_stats_t *statsp;

    statsp = mt_stats_init(1, 8, 10);

    // datapoint #1
    statsp->stats[0][7][0].msf_edges = 446724 ;
    statsp->stats[0][7][0].cycles_main = 552159;
    statsp->stats[0][7][0].cycles_helper = 7775101;
    // datapoint #2
    statsp->stats[0][7][1].msf_edges = 500010;
    statsp->stats[0][7][1].cycles_main = 955271;
    statsp->stats[0][7][1].cycles_helper = 8155960;
    // datapoint #3
    statsp->stats[0][7][2].msf_edges = 514117;
    statsp->stats[0][7][2].cycles_main = 1082033;
    statsp->stats[0][7][2].cycles_helper = 8136795;
    // datapoint #4
    statsp->stats[0][7][3].msf_edges = 519333;
    statsp->stats[0][7][3].cycles_main = 1153304;
    statsp->stats[0][7][3].cycles_helper = 8119074;
    // datapoint #5
    statsp->stats[0][7][4].msf_edges = 521663;
    statsp->stats[0][7][4].cycles_main = 1208452;
    statsp->stats[0][7][4].cycles_helper = 8064303;
    // datapoint #6
    statsp->stats[0][7][5].msf_edges = 522785;
    statsp->stats[0][7][5].cycles_main = 1264006;
    statsp->stats[0][7][5].cycles_helper = 8052788;
    // datapoint #7
    statsp->stats[0][7][6].msf_edges = 523385;
    statsp->stats[0][7][6].cycles_main = 1300835;
    statsp->stats[0][7][6].cycles_helper = 8071926;
    // datapoint #8
    statsp->stats[0][7][7].msf_edges = 523725;
    statsp->stats[0][7][7].cycles_main = 1341465;
    statsp->stats[0][7][7].cycles_helper = 8073874;
    // datapoint #9
    statsp->stats[0][7][8].msf_edges = 523918;
    statsp->stats[0][7][8].cycles_main = 1377322;
    statsp->stats[0][7][8].cycles_helper = 8087427;
    // datapoint #10
    statsp->stats[0][7][9].msf_edges = 524048;
    statsp->stats[0][7][9].cycles_main = 1410357;
    statsp->stats[0][7][9].cycles_helper = 8054422;
   
    mt_stats_partial_progress(statsp, 0, 7, 9988827, 524048);

    mt_stats_destroy(statsp);

    return 0;
}
