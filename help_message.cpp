//
// Created by sky on 2023/6/2.
//

#include "help_message.h"
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>

const char *file_path = "./help.html";
std::string msg;

const std::string& load_help_html_file() {
    std::ifstream inputFile(file_path);
    if (inputFile.is_open()) {
        std::stringstream buffer;
        buffer << inputFile.rdbuf();
        msg = buffer.str();
        inputFile.close();
    } else {
        std::cout << "failed to open file. use default help message." << std::endl;
        msg = default_help_message;
    }

    return msg;
}