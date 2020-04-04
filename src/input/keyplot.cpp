#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <ncpp/NotCurses.hh>

#define NANOSECS_IN_SEC 1000000000

static inline uint64_t
timenow_to_ns(){
  struct timespec t;
  if(clock_gettime(CLOCK_MONOTONIC, &t)){
    throw std::runtime_error("error retrieving time");
  }
  return t.tv_sec * NANOSECS_IN_SEC + t.tv_nsec;
}

std::mutex mtx;
uint64_t start;
std::atomic<bool> done;
std::array<struct ncplot*, 5> plots;

void Tick(ncpp::NotCurses* nc, uint64_t sec) {
  const std::lock_guard<std::mutex> lock(mtx);
  for(auto i = 0u ; i < plots.size() ; ++i){
    if(ncplot_add_sample(plots[i], sec, 0)){
      throw std::runtime_error("couldn't register timetick");
    }
  }
  if(!nc->render()){
    throw std::runtime_error("error rendering");
  }
  mtx.unlock();
}

void Ticker(ncpp::NotCurses* nc) {
  do{
    std::this_thread::sleep_for(std::chrono::seconds{1});
    const uint64_t sec = (timenow_to_ns() - start) / NANOSECS_IN_SEC;
    Tick(nc, sec);
  }while(!done);
}

int main(void){
  if(setlocale(LC_ALL, "") == nullptr){
    return EXIT_FAILURE;
  }
  srand(time(NULL));
  ncpp::NotCurses nc;
  if(!nc.mouse_enable()){
    return EXIT_FAILURE;
  }
  std::unique_ptr<ncpp::Plane> n(nc.get_stdplane());
  ncpp::Cell tl, tr, bl, br, hl, vl;
  if(!n->load_double_box(0, 0, tl, tr, bl, br, hl, vl)){
    return EXIT_FAILURE;
  }
  if(!n->perimeter(tl, tr, bl, br, hl, vl, 0)){
    return EXIT_FAILURE;
  }
  std::vector<ncpp::Plane> planes;
  const int plotlen = n->get_dim_x() - 2;
  planes.emplace_back(6, plotlen, 1,  1, nullptr);
  planes.emplace_back(6, plotlen, 8,  1, nullptr);
  planes.emplace_back(6, plotlen, 15,  1, nullptr);
  planes.emplace_back(6, plotlen, 23,  1, nullptr);
  planes.emplace_back(6, plotlen, 31,  1, nullptr);
  struct ncplot_options popts{};
  for(auto i = 0u ; i < plots.size() ; ++i){
    if(i == plots.size() - 1){
      popts.labelaxisd = true;
    }
    popts.minchannel = popts.maxchannel = 0;
    channels_set_fg_rgb(&popts.maxchannel, random() % 256, random() % 256, random() % 256);
    channels_set_fg_rgb(&popts.minchannel, random() % 256, random() % 256, random() % 256);
    popts.gridtype = static_cast<ncgridgeom_e>(i);
    plots[i] = ncplot_create(planes[i], &popts);
  }
  char32_t r;
  ncinput ni;
  if(!nc.render()){
    return EXIT_FAILURE;
  }
  done = false;
  start = timenow_to_ns();
  std::thread tid(Ticker, &nc);
  while(errno = 0, (r = nc.getc(true, &ni)) != (char32_t)-1){
    if(r == 0){ // interrupted by signal
      continue;
    }
    const uint64_t sec = (timenow_to_ns() - start) / NANOSECS_IN_SEC;
    mtx.lock();
    for(auto i = 0u ; i < plots.size() ; ++i){
      if(ncplot_add_sample(plots[i], sec, 1)){
        return EXIT_FAILURE;
      }
    }
    if(!nc.render()){
      return EXIT_FAILURE;
    }
    mtx.unlock();
  }
  return EXIT_SUCCESS;
}
