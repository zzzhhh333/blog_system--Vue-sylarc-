<template>
  <div class="blog-detail" v-if="blog">
    <el-card>
      <template #header>
        <div class="blog-header">
          <h1>{{ blog.title }}</h1>
          <div class="blog-meta">
            <span class="author">作者: {{ blog.author_name }}</span>
            <span class="time">发布时间: {{ formatTime(blog.created_at) }}</span>
            <span class="views">阅读量: {{ blog.view_count }}</span>
          </div>
        </div>
      </template>

      <div class="blog-content">
        <div v-if="blog.content" class="content-text">
          {{ blog.content }}
        </div>
        <div v-else class="no-content">
          暂无内容
        </div>
      </div>

      <template #footer>
        <div class="blog-actions">
          <el-button @click="$router.go(-1)">返回</el-button>
          <el-button 
            v-if="isMyBlog" 
            type="primary" 
            @click="handleEdit"
          >
            编辑
          </el-button>
          <el-button 
            v-if="isMyBlog" 
            type="danger" 
            @click="handleDelete"
          >
            删除
          </el-button>
        </div>
      </template>
    </el-card>

    <BlogEditor v-model="showEditor" :blog="blog" @success="handleEditorSuccess" />
  </div>

  <div v-else-if="loading" class="loading">
    <el-skeleton :rows="10" animated />
  </div>

  <div v-else class="not-found">
    <el-result icon="warning" title="博客不存在">
      <template #extra>
        <el-button type="primary" @click="$router.push('/')">返回首页</el-button>
      </template>
    </el-result>
  </div>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { useAuthStore } from '../stores/auth'
import { getBlogDetail, deleteBlog } from '../api/request'
import { ElMessage, ElMessageBox } from 'element-plus'
import BlogEditor from '../components/BlogEditor.vue'

const route = useRoute()
const router = useRouter()
const authStore = useAuthStore()

const blog = ref(null)
const loading = ref(false)
const showEditor = ref(false)

const blogId = computed(() => route.params.id)

const isMyBlog = computed(() => {
  return authStore.isAuthenticated && blog.value?.author_id === authStore.user?.id
})

const fetchBlogDetail = async () => {
  if (!blogId.value) return

  loading.value = true
  try {
    const response = await getBlogDetail(blogId.value)
    if (response.data.code === 0) {
      blog.value = response.data.data
    } else {
      blog.value = null
    }
  } catch (error) {
    ElMessage.error('获取博客详情失败')
    blog.value = null
  } finally {
    loading.value = false
  }
}

const handleEdit = () => {
  showEditor.value = true
}

const handleDelete = async () => {
  try {
    await ElMessageBox.confirm('确定要删除这篇博客吗？此操作不可恢复。', '警告', {
      type: 'warning',
      confirmButtonText: '确定删除',
      cancelButtonText: '取消'
    })

    const response = await deleteBlog(blogId.value)
    if (response.data.code === 0) {
      ElMessage.success('删除成功')
      router.push('/')
    } else {
      ElMessage.error('删除失败')
    }
  } catch (error) {
    if (error !== 'cancel') {
      ElMessage.error('删除失败')
    }
  }
}

const handleEditorSuccess = () => {
  showEditor.value = false
  fetchBlogDetail()
}

const formatTime = (timestamp) => {
  return new Date(timestamp * 1000).toLocaleString()
}

onMounted(() => {
  fetchBlogDetail()
})
</script>

<style scoped>
.blog-detail {
  max-width: 1000px;
  margin: 0 auto;
  padding: 20px;
}

.blog-header h1 {
  margin: 0 0 15px;
  font-size: 2em;
  color: #333;
  line-height: 1.4;
}

.blog-meta {
  display: flex;
  gap: 20px;
  color: #666;
  font-size: 0.9em;
}

.blog-content {
  line-height: 1.8;
  font-size: 1.1em;
  color: #444;
}

.content-text {
  white-space: pre-wrap;
  word-break: break-word;
}

.no-content {
  text-align: center;
  color: #999;
  padding: 60px 0;
  font-style: italic;
}

.blog-actions {
  display: flex;
  gap: 10px;
  justify-content: center;
}

.loading {
  max-width: 1000px;
  margin: 0 auto;
  padding: 20px;
}

.not-found {
  max-width: 600px;
  margin: 100px auto;
}
</style>