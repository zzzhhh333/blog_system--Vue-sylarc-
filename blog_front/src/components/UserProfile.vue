<template>
  <el-card class="profile-card">
    <template #header>
      <div class="profile-header">
        <h2>个人资料</h2>
      </div>
    </template>

    <div v-if="!authStore.user" class="loading-state">
      <el-skeleton :rows="5" animated />
    </div>

    <el-form v-else :model="form" :rules="rules" ref="formRef" label-width="80px">
      <el-form-item label="用户名">
        <el-input :value="authStore.user.username || '未设置'" disabled />
        <div class="form-tip">用户名不可修改</div>
      </el-form-item>

      <el-form-item label="昵称" prop="nickname">
        <el-input 
          v-model="form.nickname" 
          placeholder="请输入昵称" 
          :disabled="loading"
        />
      </el-form-item>

      <el-form-item label="邮箱" prop="email">
        <el-input 
          v-model="form.email" 
          placeholder="请输入邮箱地址" 
          :disabled="loading"
        />
      </el-form-item>

      <el-form-item label="头像" prop="avatar">
        <el-input 
          v-model="form.avatar" 
          placeholder="请输入头像URL" 
          :disabled="loading"
        />
        <div class="form-tip">支持网络图片链接</div>
        <div v-if="form.avatar" class="avatar-preview">
          <img :src="form.avatar" alt="头像预览" @error="handleImageError" />
        </div>
      </el-form-item>

      <el-form-item label="个人简介" prop="bio">
        <el-input
          v-model="form.bio"
          type="textarea"
          :rows="4"
          placeholder="请输入个人简介"
          show-word-limit
          maxlength="200"
          :disabled="loading"
        />
      </el-form-item>

      <el-form-item>
        <el-button type="primary" :loading="loading" @click="handleSubmit">
          保存修改
        </el-button>
        <el-button @click="resetForm" :disabled="loading">重置</el-button>
      </el-form-item>
    </el-form>
  </el-card>
</template>

<script setup>
import { ref, reactive, watch, onMounted, nextTick } from 'vue'
import { useAuthStore } from '../stores/auth'
import { updateProfile } from '../api/request'
import { ElMessage } from 'element-plus'

const authStore = useAuthStore()
const formRef = ref()
const loading = ref(false)
const isFormInitialized = ref(false)

const form = reactive({
  nickname: '',
  email: '',
  avatar: '',
  bio: ''
})

const rules = {
  nickname: [
    { required: true, message: '请输入昵称', trigger: 'blur' },
    { min: 2, max: 20, message: '昵称长度在 2 到 20 个字符', trigger: 'blur' }
  ],
  email: [
    { required: true, message: '请输入邮箱地址', trigger: 'blur' },
    { type: 'email', message: '请输入正确的邮箱地址', trigger: 'blur' }
  ],
  bio: [
    { max: 200, message: '个人简介不能超过200个字符', trigger: 'blur' }
  ]
}

const initForm = () => {
  if (!authStore.user) {
    console.warn('用户数据未加载，无法初始化表单')
    return false
  }

  form.nickname = authStore.user.nickname || ''
  form.email = authStore.user.email || ''
  form.avatar = authStore.user.avatar || ''
  form.bio = authStore.user.bio || ''
  
  isFormInitialized.value = true
  return true
}

const handleSubmit = async () => {
  if (!formRef.value) {
    ElMessage.warning('表单未初始化')
    return
  }

  try {
    const valid = await formRef.value.validate()
    if (!valid) {
      ElMessage.warning('请完善表单信息')
      return
    }

    loading.value = true
    const response = await updateProfile(form)
    
    if (response.data.code === 200) {
      ElMessage.success('资料更新成功')
      await authStore.fetchUserInfo()
    } else {
      ElMessage.error(response.data.message || '更新失败')
    }
  } catch (error) {
    console.error('更新个人资料失败:', error)
    if (error.response?.data?.message) {
      ElMessage.error(error.response.data.message)
    } else {
      ElMessage.error('更新失败，请检查网络连接')
    }
  } finally {
    loading.value = false
  }
}

const resetForm = () => {
  if (initForm() && formRef.value) {
    formRef.value.clearValidate()
    ElMessage.info('表单已重置')
  }
}

const handleImageError = (event) => {
  ElMessage.warning('头像图片加载失败，请检查URL是否正确')
  event.target.style.display = 'none'
}

// 初始化
onMounted(() => {
  if (authStore.user) {
    initForm()
  }
})

// 监听用户数据变化
watch(() => authStore.user, (newUser) => {
  if (newUser && !isFormInitialized.value) {
    nextTick(initForm)
  }
})
</script>

<style scoped>
.profile-card {
  max-width: 600px;
  margin: 0 auto;
}

.profile-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.profile-header h2 {
  margin: 0;
  color: #409eff;
}

.form-tip {
  font-size: 0.8em;
  color: #999;
  margin-top: 5px;
}

.avatar-preview {
  margin-top: 10px;
}

.avatar-preview img {
  max-width: 100px;
  max-height: 100px;
  border-radius: 50%;
  border: 1px solid #ddd;
}

.loading-state {
  padding: 20px;
}

:deep(.el-form-item__label) {
  font-weight: 500;
}
</style>