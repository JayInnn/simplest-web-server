#include <sstream>
#include "http_response.h"


const std::unordered_map<std::string, std::string> http_response::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};

const std::unordered_map<int, std::string> http_response::CODE_STATUS = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
};

const std::unordered_map<int, std::string> http_response::CODE_PATH = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" },
};


void http_response::set_error_info() {
    rsp_keepalive = false;
    rsp_code = 400;
    rsp_path = "/400.html";
    stat((rsp_resource_path + rsp_path).data(), &m_file_stat);
}

void http_response::set_finish_info(std::string path, bool keepalive) {
    /* 判断请求的资源文件 */
    if(stat((rsp_resource_path + rsp_path).data(), &m_file_stat) < 0 || S_ISDIR(m_file_stat.st_mode)) {
        rsp_code = 404;
    }
    else if(!(m_file_stat.st_mode & S_IROTH)) {
        rsp_code = 403;
    }
    else if(rsp_code == -1) { 
        rsp_code = 200; 
    }

    if(CODE_PATH.count(rsp_code) == 1) {
        rsp_path = CODE_PATH.find(rsp_code)->second;
        stat((rsp_resource_path + rsp_path).data(), &m_file_stat);
    } else {
        rsp_path = path;
    }
    rsp_keepalive = keepalive;
}


std::string http_response::build_response_body() {
    std::stringstream response_stream;
    // add response line
    std::string status;
    if(CODE_STATUS.count(rsp_code) == 1) {
        status = CODE_STATUS.find(rsp_code)->second;
    } else {
        rsp_code = 400;
        status = CODE_STATUS.find(400)->second;
    }
    response_stream << "HTTP/1.1 " << rsp_code << " " << status << "\r\n";

    // add response header
    if(rsp_keepalive) {
        response_stream << "Connection: keep-alive\r\n";
        response_stream << "keep-alive: max=6, timeout=120\r\n";
    } else{
        response_stream << "Connection: close\r\n";
    }
    response_stream << "Content-type: " << get_content_type() << "\r\n";

    // add response content
    int res_fd = open((rsp_resource_path + rsp_path).data(), O_RDONLY);
    if(res_fd < 0) { 
        std::string err_msg = error_content("File NotFound!");
        response_stream << "Content-length: " << err_msg.size() << "\r\n\r\n";
        response_stream << err_msg;
        return response_stream.str();
    }

    /* 将文件映射到内存提高文件的访问速度, MAP_PRIVATE 建立一个写入时拷贝的私有映射*/
    LOG_DEBUG("file path %s", (rsp_resource_path + rsp_path).data());
    int* m_ret = (int*)mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, res_fd, 0);
    if(*m_ret == -1) {
        std::string tmp("File NotFound!");
        std::string err_msg = error_content(tmp);
        response_stream << "Content-length: " << err_msg.size() << "\r\n\r\n";
        response_stream << err_msg;
        return response_stream.str();
    }
    m_file = (char*)m_ret;
    close(res_fd);
    response_stream << "Content-length: " << m_file_stat.st_size << "\r\n\r\n";
    return response_stream.str();
}

char* http_response::get_m_file() {
    return m_file;
}

size_t http_response::get_file_len() const {
    return m_file_stat.st_size;
}

/**
 * private method
 */

std::string http_response::get_content_type() {
    /* 判断文件类型 */
    std::string::size_type idx = rsp_path.find_last_of('.');
    if(idx == std::string::npos) {
        return "text/plain";
    }
    std::string suffix = rsp_path.substr(idx);
    if(SUFFIX_TYPE.count(suffix) == 1) {
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}

std::string http_response::error_content(std::string message) {
    std::string status;
    if(CODE_STATUS.count(rsp_code) == 1) {
        status = CODE_STATUS.find(rsp_code)->second;
    } else {
        status = "Bad Request";
    }
    std::stringstream body_stream;
    body_stream << "<html><title>Error</title>"
                << "<body bgcolor=\"ffffff\">" << rsp_code << " : " << status << "\n"
                << "<p>" << message << "</p>"
                << "<hr><em>simplest web server</em></body></html>";
    return body_stream.str();
}


