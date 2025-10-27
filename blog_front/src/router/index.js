import { createRouter, createWebHistory } from 'vue-router'
import Home from '../views/Home.vue'
import Login from '../views/Login.vue'
import Register from '../views/Register.vue'
import Dashboard from '../views/Dashboard.vue'
import BlogDetail from '../views/BlogDetail.vue'

const routes = [
  { path: '/', name: 'Home', component: Home },
  { path: '/login', name: 'Login', component: Login },
  { path: '/register', name: 'Register', component: Register },
  { path: '/dashboard', name: 'Dashboard', component: Dashboard, meta: { requiresAuth: true } },
  { path: '/blog/:id', name: 'BlogDetail', component: BlogDetail }
]

const router = createRouter({
  history: createWebHistory(),
  routes
})

// 修复路由守卫：直接检查 token
router.beforeEach((to, from, next) => {
  console.log('路由守卫检查:', {
    路径: to.path,
    需要认证: to.meta.requiresAuth,
    token是否存在: !!localStorage.getItem('token')
  })
  
  const token = localStorage.getItem('token')
  const isAuthenticated = !!token  // 直接检查 token 是否存在
  
  if (to.meta.requiresAuth && !isAuthenticated) {
    console.log('未认证，重定向到登录页')
    next('/login')
  } else {
    console.log('允许访问')
    next()
  }
})

export default router