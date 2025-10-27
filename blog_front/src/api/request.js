import axios from 'axios'

const api = axios.create({
  baseURL: '/api',
  timeout: 10000
})

api.interceptors.request.use(
  (config) => {
    const token = localStorage.getItem('token')
    if (token) {
      config.headers.Authorization = `Bearer ${token}`
    }
    return config
  },
  (error) => {
    return Promise.reject(error)
  }
)

api.interceptors.response.use(
  (response) => {
    return response
  },
  (error) => {
    if (error.response?.status === 401) {
      localStorage.removeItem('token')
      window.location.href = '/login'
    }
    return Promise.reject(error)
  }
)

export const login = (username, password) => {
  return api.post('/users/login', { username, password })
}

export const register = (userData) => {
  return api.post('/users/register', userData)
}

export const getUserInfo = (userId = null) => {
  const params = userId ? { user_id: userId } : {}
  return api.get('/users/info', { params })
}

export const updateProfile = (profileData) => {
  return api.post('/users/profile', profileData)
}

export const getBlogList = (page = 1, pageSize = 10, status = 1) => {
  return api.get('/blogs', {
    params: { page, page_size: pageSize, status }
  })
}

export const getBlogListByUser = (userId, page = 1, pageSize = 10) => {
  console.log('Fetching blogs for user:', userId, 'Page:', page, 'Page Size:', pageSize);
  return api.get('/blogs/user', {
    params: { user_id: userId, page: page, page_size: pageSize }
  })
}

export const getBlogDetail = (id) => {
  return api.get('/blogs/detail', {
    params: { id }
  })
}

export const createBlog = (blogData) => {
  return api.post('/blogs/create', blogData)
}

export const updateBlog = (blogData) => {
  return api.post('/blogs/update', blogData)
}

export const deleteBlog = (id) => {
  return api.get('/blogs/delete', {
    params: { id }
  })
}

export default api