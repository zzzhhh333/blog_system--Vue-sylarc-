<template>
  <div class="dashboard">
    <el-tabs v-model="activeTab" type="card" @tab-change="handleTabChange">
      <el-tab-pane label="我的博客" name="blogs">
        <div class="tab-content">
          <el-button 
            type="primary" 
            @click="showEditor = true" 
            style="margin-bottom: 20px;"
          >
            <el-icon><Plus /></el-icon>
            写博客
          </el-button>
          <BlogList :show-user-only="true" />
        </div>
      </el-tab-pane>
      
      <el-tab-pane label="个人资料" name="profile">
        <div class="tab-content">
          <UserProfile />
        </div>
      </el-tab-pane>
    </el-tabs>
    
    <BlogEditor v-model="showEditor" @success="handleBlogCreated" />
  </div>
</template>

<script setup>
import { ref, watch, onMounted } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { Plus } from '@element-plus/icons-vue'
import BlogList from '../components/BlogList.vue'
import BlogEditor from '../components/BlogEditor.vue'
import UserProfile from '../components/UserProfile.vue'

const route = useRoute()
const router = useRouter()

// 从路由查询参数中获取激活的标签页
const activeTab = ref(route.query.tab || 'blogs')
const showEditor = ref(false)

// 监听标签页变化，更新路由
const handleTabChange = (tabName) => {
  router.replace({ 
    query: { ...route.query, tab: tabName }
  })
}

// 监听路由变化，同步标签页状态
watch(
  () => route.query.tab,
  (newTab) => {
    if (newTab) {
      activeTab.value = newTab
    }
  }
)

const handleBlogCreated = () => {
  showEditor.value = false
}

// 页面加载时恢复状态
onMounted(() => {
  if (route.query.tab) {
    activeTab.value = route.query.tab
  }
})
</script>

<style scoped>
.dashboard {
  max-width: 1200px;
  margin: 0 auto;
  padding: 20px;
}

.tab-content {
  padding: 20px 0;
}
</style>