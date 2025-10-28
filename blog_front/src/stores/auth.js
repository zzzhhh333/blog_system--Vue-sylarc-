import { defineStore } from 'pinia'
import { ref } from 'vue'
import { login, register, getUserInfo } from '../api/request'

export const useAuthStore = defineStore('auth', () => {
  const user = ref(null)
  const token = ref(localStorage.getItem('token'))
  const isAuthenticated = ref(!!token.value)

  // 初始化时尝试获取用户信息
  if (token.value) {
    getUserInfo().then(response => {
      if (response.data.code === 200) {
        user.value = response.data.data
      }
    }).catch(() => {
      logout()
    })
  }

  const loginUser = async (username, password) => {
    try {
      const response = await login(username, password)
      if (response.data.code === 200) {
        token.value = response.data.data.token
        user.value = response.data.data.user
        localStorage.setItem('token', token.value)
        isAuthenticated.value = true
        return true
      }
      return false
    } catch (error) {
      console.error('Login failed:', error)
      return false
    }
  }

  const registerUser = async (userData) => {
    try {
      const response = await register(userData)
      if (response.data.code === 200) {
        token.value = response.data.data.token
        user.value = response.data.data.user
        localStorage.setItem('token', token.value)
        isAuthenticated.value = true
        return true
      }
      return false
    } catch (error) {
      console.error('Registration failed:', error)
      return false
    }
  }

  const logout = () => {
    user.value = null
    token.value = null
    isAuthenticated.value = false
    localStorage.removeItem('token')
  }

  const fetchUserInfo = async () => {
    if (token.value) {
      try {
        const response = await getUserInfo()
        if (response.data.code === 200) {
          user.value = response.data.data
        }
      } catch (error) {
        console.error('Failed to fetch user info:', error)
        logout()
      }
    }
  }

  return {
    user,
    token,
    isAuthenticated,
    loginUser,
    registerUser,
    logout,
    fetchUserInfo
  }
})