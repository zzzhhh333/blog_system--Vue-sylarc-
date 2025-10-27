<template>
  <div class="dashboard">
    <el-tabs v-model="activeTab" type="card">
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
import { ref } from 'vue'
import { Plus } from '@element-plus/icons-vue'
import BlogList from '../components/BlogList.vue'
import BlogEditor from '../components/BlogEditor.vue'
import UserProfile from '../components/UserProfile.vue'

const activeTab = ref('blogs')
const showEditor = ref(false)

const handleBlogCreated = () => {
  showEditor.value = false
}
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