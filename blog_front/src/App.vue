<template>
  <div id="app">
    <el-container>
      <el-header>
        <nav class="navbar">
          <div class="nav-brand">
            <router-link to="/">博客系统</router-link>
          </div>
          <div class="nav-links">
            <router-link to="/">首页</router-link>
            <template v-if="authStore.isAuthenticated">
              <router-link to="/dashboard">我的空间</router-link>
              <el-dropdown>
                <span class="el-dropdown-link">
                  {{ authStore.user?.nickname || authStore.user?.username }}
                  <el-icon><arrow-down /></el-icon>
                </span>
                <template #dropdown>
                  <el-dropdown-menu>
                    <el-dropdown-item @click="handleLogout">退出登录</el-dropdown-item>
                  </el-dropdown-menu>
                </template>
              </el-dropdown>
            </template>
            <template v-else>
              <router-link to="/login">登录</router-link>
              <router-link to="/register">注册</router-link>
            </template>
          </div>
        </nav>
      </el-header>
      <el-main>
        <router-view />
      </el-main>
    </el-container>
  </div>
</template>

<script setup>
import { useAuthStore } from './stores/auth'
import { ArrowDown } from '@element-plus/icons-vue'

const authStore = useAuthStore()

const handleLogout = () => {
  authStore.logout()
  window.location.href = '/'
}
</script>

<style scoped>
.navbar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 0 20px;
  background: #409eff;
  color: white;
  height: 60px;
}

.nav-brand a {
  color: white;
  text-decoration: none;
  font-size: 1.5em;
  font-weight: bold;
}

.nav-links {
  display: flex;
  gap: 20px;
  align-items: center;
}

.nav-links a {
  color: white;
  text-decoration: none;
}

.el-dropdown-link {
  color: white;
  cursor: pointer;
}
</style>

<style>
* {
  margin: 0;
  padding: 0;
  box-sizing: border-box;
}

body {
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
}

#app {
  min-height: 100vh;
}
</style>