#include <iostream>

#include <socketcan_interface/threading.h>
#include <socketcan_interface/socketcan.h>
#include <socketcan_interface/make_shared.h>

#include <boost/atomic.hpp>

boost::atomic<size_t> g_counter;
can::Frame g_msg(can::MsgHeader(0, true));

bool next_message(can::CommInterface &writer) {
    g_msg.id = g_counter & ((1<<11)-1);
    if(!writer.send(g_msg)) return false;
    ++g_counter;
    return true;
}

void process_frame(const can::Frame &msg, can::CommInterface &writer) {
    if(g_msg == msg) {
      next_message(writer);
    }
}

int main(int argc, char *argv[]){

    if(argc != 3){
        std::cout << "usage: "<< argv[0] << " CAN_OUT CAN_IN" << std::endl;
        return 1;
    }

    can::ThreadedSocketCANInterface driver_out;
    can::ThreadedSocketCANInterface driver_in;


    if(!driver_out.init(argv[1], false)){
      std::cerr << "Could not initialize output '" << argv[1] << "'" << std::endl;
        return 1;
    }

    if(!driver_in.init(argv[2], false)){
      std::cerr << "Could not initialize input '" << argv[2] << "'" << std::endl;
        return 1;
    }
    can::FrameListenerConstSharedPtr listener = driver_in.createMsgListener(boost::bind(process_frame, boost::placeholders::_1, boost::ref(static_cast<can::CommInterface&>(driver_out))));

    boost::chrono::steady_clock::time_point start = boost::chrono::steady_clock::now();
    if(!next_message(driver_out)){
      std::cerr << "Could not send message" << std::endl;
      return 1;
    }

    while (true) {
      boost::this_thread::sleep_for(boost::chrono::seconds(10));
      boost::chrono::steady_clock::time_point now = boost::chrono::steady_clock::now();
      size_t counter = g_counter;
      double diff = boost::chrono::duration_cast<boost::chrono::duration<double> >(now-start).count();
      std::cout << diff << "\t" << counter << "\t" << counter / diff << std::endl;
    }

    return 0;

}
