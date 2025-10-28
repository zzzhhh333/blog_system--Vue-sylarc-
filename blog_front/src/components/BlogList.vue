<template>
  <div class="blog-list">
    <div v-if="loading" class="loading">
      <el-skeleton :rows="5" animated />
    </div>
    
    <div v-else-if="blogs.length === 0" class="empty">
      <el-empty description="暂无博客" />
    </div>
    
    <div v-else class="blogs">
      <div v-for="blog in blogs" :key="blog.id" class="blog-item">
        <div class="blog-content">
          <h3 @click="viewBlog(blog.id)" class="blog-title">{{ blog.title }}</h3>
          <p class="blog-summary">{{ blog.summary || blog.content.substring(0, 100) + '...' }}</p>
          <div class="blog-meta">
            <span class="author">作者: {{ blog.author_name }}</span>
            <span class="time">{{ formatTime(blog.created_at) }}</span>
            <span class="views">阅读: {{ blog.view_count }}</span>
          </div>
        </div>
        
        <div v-if="isMyBlog(blog)" class="blog-actions">
          <el-button size="small" @click="editBlog(blog)">编辑</el-button>
          <el-button size="small" type="danger" @click="deleteBlog(blog.id)">删除</el-button>
        </div>
      </div>
    </div>
    
    <el-pagination
      v-if="total > pageSize"
      v-model:current-page="currentPage"
      :page-size="pageSize"
      :total="total"
      layout="prev, pager, next"
      @current-change="handlePageChange"
    />
    
    <BlogEditor v-model="showEditor" :blog="editingBlog" @success="handleEditorSuccess" />
  </div>
</template>

<script setup>
import { ref, computed, onMounted, watch } from 'vue'
import { useRouter } from 'vue-router'
import { useAuthStore } from '../stores/auth'
import { getBlogList,getBlogListByUser,deleteBlog as deleteBlogApi } from '../api/request'
import { ElMessage, ElMessageBox } from 'element-plus'
import BlogEditor from './BlogEditor.vue'

const props = defineProps({
  showUserOnly: {
    type: Boolean,
    default: false
  }
})

const router = useRouter()
const authStore = useAuthStore()
const blogs = ref([])
const loading = ref(false)
const currentPage = ref(1)
const pageSize = ref(10)
const total = ref(0)
const showEditor = ref(false)
const editingBlog = ref(null)

const fetchBlogs = async () => {
  loading.value = true
  try {
    let response
    


    if (props.showUserOnly){
      // 检查用户信息是否存在
      if (!authStore.user || !authStore.user.id) {
        console.error('用户信息不存在，无法获取用户博客')
        ElMessage.warning('用户信息加载中，请稍后...')
        return
      }
      console.log('Fetching blogs for user ID:', authStore.user.id, 'Page:', currentPage.value, 'Page Size:', pageSize.value); 
      response = await getBlogListByUser(authStore.user.id, currentPage.value, pageSize.value)
    }
    else{
      const status = 1
      response = await getBlogList(currentPage.value, pageSize.value, status)
    }
    if (response.data.code === 200) {
      blogs.value = response.data.data
      total.value = response.data.data.length
    }
  } catch (error) {
    console.error('原始方法错误:', error)
    ElMessage.error('获取博客列表失败')
  } finally {
    loading.value = false
  }
}

const isMyBlog = (blog) => {
  return authStore.isAuthenticated && blog.author_id === authStore.user?.id
}

const viewBlog = (id) => {
  // 改为使用路由跳转
  router.push(`/blog/${id}`)
}

const editBlog = (blog) => {
  editingBlog.value = blog
  showEditor.value = true
}

const deleteBlog = async (id) => {
  try {
    await ElMessageBox.confirm('确定要删除这篇博客吗？', '提示', {
      type: 'warning'
    })
    
    const response = await deleteBlogApi(id)
    if (response.data.code === 200) {
      ElMessage.success('删除成功')
      fetchBlogs()
    }
  } catch (error) {
    if (error !== 'cancel') {
      ElMessage.error('删除失败')
    }
  }
}

const handlePageChange = (page) => {
  currentPage.value = page
  fetchBlogs()
}

const handleEditorSuccess = () => {
  showEditor.value = false
  editingBlog.value = null
  fetchBlogs()
}

const formatTime = (timestamp) => {
  return new Date(timestamp * 1000).toLocaleString()
}

onMounted(() => {
  fetchBlogs()
})

watch(() => props.showUserOnly, () => {
  currentPage.value = 1
  fetchBlogs()
})
</script>

<style scoped>
.blog-item {
  display: flex;
  justify-content: between;
  padding: 20px 0;
  border-bottom: 1px solid #eee;
}

.blog-content {
  flex: 1;
}

.blog-title {
  margin: 0 0 10px;
  color: #409eff;
  cursor: pointer;
}

.blog-title:hover {
  text-decoration: underline;
}

.blog-summary {
  color: #666;
  margin-bottom: 10px;
  line-height: 1.6;
}

.blog-meta {
  font-size: 0.9em;
  color: #999;
}

.blog-meta span {
  margin-right: 15px;
}

.blog-actions {
  display: flex;
  align-items: flex-start;
  gap: 10px;
}

.loading, .empty {
  padding: 40px 0;
  text-align: center;
}

.el-pagination {
  justify-content: center;
  margin-top: 20px;
}
</style>