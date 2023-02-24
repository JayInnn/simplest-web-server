#include "http_session.h"
#include <string>
#include <regex>
#include <sys/socket.h>

http_session::http_session(int fd_, uint32_t event, std::shared_ptr<epoller>& epl): 
    fd(fd_), conn_event(event), epler_(epl) {}


bool http_session::read_buf() {
    int  recv_cnt = 0;
    /**
     * recv函数返回说明：
     *     <0 出错；
     *     =0 连接关闭；
     *     >0 接收到的数据长度大小；
     */
    do {
        recv_cnt = recv(fd, &m_read_buf[m_read_idx], READ_BUFFER_SIZE - m_read_idx, 0);
        if (recv_cnt == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            return false;
        }
        else if (recv_cnt == 0)
        {
            return false;
        }
        m_read_idx += recv_cnt;
    }while(conn_event & EPOLLET);

    return true;
}


/**
 * @return
 *      true 继续打开连接
 *      false 关闭连接
 */
bool http_session::write_buf() {
    int temp = 0;

    while (1) {
        temp = writev(fd, iov_vec, iov_cnt);
        if (temp < 0) {
            if (errno == EAGAIN) {
                epler_->mod_fd(fd, conn_event | EPOLLOUT);
                return true;
            }
            return false;
        }

        bytes_have_send += temp;
        bytes_to_send -= temp;
        if (bytes_have_send >= iov_vec[0].iov_len) {
            iov_vec[0].iov_len = 0;
            iov_vec[1].iov_base = (uint8_t*)iov_vec[1].iov_base + (bytes_have_send - iov_vec[0].iov_len);
            iov_vec[1].iov_len = bytes_to_send;
        } else {
            iov_vec[0].iov_base = (uint8_t*)iov_vec[0].iov_base + bytes_have_send;
            iov_vec[0].iov_len = iov_vec[0].iov_len - bytes_have_send;
        }

        if (bytes_to_send <= 0) {
            if(request.get_keepalive()) {
                epler_->mod_fd(fd, conn_event | EPOLLIN);
                return true;
            } else {
                return false;
            }
        }
    }
}

void http_session::process() {
    process_read_buf();
    if(m_check_state == CHECK_STATE_ERROR || m_check_state == CHECK_STATE_FINISH) {
        if(m_check_state == CHECK_STATE_ERROR) {
            response.set_error_info();
        }
        if(m_check_state == CHECK_STATE_FINISH) {
            response.set_finish_info(request.get_path(), request.get_keepalive());
        }

        std::string body = response.build_response_body();
        /* 响应头 */
        iov_vec[0].iov_base = const_cast<char*>(body.c_str());
        iov_vec[0].iov_len = body.size();
        iov_cnt = 1;
        bytes_to_send += iov_vec[0].iov_len;
        /* 文件 */
        if(response.get_file_len() > 0  && response.get_m_file()) {
            iov_vec[1].iov_base = response.get_m_file();
            iov_vec[1].iov_len = response.get_file_len();
            iov_cnt = 2;
            bytes_to_send += iov_vec[1].iov_len;
        }

        if(bytes_to_send > 0) {
            epler_->mod_fd(fd, conn_event | EPOLLOUT);
        }

        return;
    }
    epler_->mod_fd(fd, conn_event | EPOLLIN);
}


void http_session::process_read_buf() {
    while (m_check_state != CHECK_STATE_FINISH && m_check_state != CHECK_STATE_ERROR && parse_line()) {
        std::string text(m_read_buf, m_start_line);
        LOG_INFO("fd: %d, process_read buffer: %s", fd, text.c_str());
        m_start_line = m_checked_idx;
        switch (m_check_state) {
            case CHECK_STATE_REQUESTLINE: {
                if(!parse_request_line(text)) {
                    m_check_state = CHECK_STATE_ERROR;
                    LOG_ERROR("fd: %d, parse request line error", fd);
                    return;
                }
                m_check_state = CHECK_STATE_HEADER;
                break;
            }
            case CHECK_STATE_HEADER: {
                if(text == "") {
                    m_check_state = CHECK_STATE_CONTENT;
                    break;
                }

                if(!parse_headers(text)) {
                    m_check_state = CHECK_STATE_ERROR;
                    return;
                }
                break;
            }
            case CHECK_STATE_CONTENT: {
                if(!parse_content(text)) {
                    m_check_state = CHECK_STATE_ERROR;
                    return;
                }
                m_check_state = CHECK_STATE_FINISH;
                break;
            }
            default: {
                m_check_state = CHECK_STATE_ERROR;
                return;
            }
        }
    }
}


// TODO: optimize, streaming parse 
bool http_session::parse_line() {
    for(int i = m_checked_idx; i < m_read_idx; ++i) {
        if (m_read_buf[i] == '\r' && m_read_buf[i + 1] == '\n') {
            m_read_buf[i++] = '\0';
            m_read_buf[i++] = '\0';
            m_checked_idx = i;
            return true;
        }
    }
    return false;
}

bool http_session::parse_request_line(std::string& line) {
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch sub_match;
    if(std::regex_match(line, sub_match, patten)) {
        request.set_request_line(sub_match[1], sub_match[2], sub_match[3]);
        return true;
    }
    return false;
}


bool http_session::parse_headers(std::string& line) {
    std::regex patten("^([^:]*): ?(.*)$");
    std::smatch sub_match;
    if(std::regex_match(line, sub_match, patten)) {
        request.set_requset_header(sub_match[1], sub_match[2]);
        return true;
    }
    return false;
}


bool http_session::parse_content(std::string& line) {
    request.set_request_body(line);
    LOG_DEBUG("fd: %d, request body:%s, len:%d", fd, line.c_str(), line.size());
}
