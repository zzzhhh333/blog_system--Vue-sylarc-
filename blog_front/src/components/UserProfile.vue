<template>
  <el-card class="profile-card">
    <template #header>
      <div class="profile-header">
        <h2>个人资料</h2>
      </div>
    </template>

    <el-form :model="form" :rules="rules" ref="formRef" label-width="80px">
      <el-form-item label="用户名">
        <el-input v-model="authStore.user.username" disabled />
        <div class="form-tip">用户名不可修改</div>
      </el-form-item>

      <el-form-item label="昵称" prop="nickname">
        <el-input v-model="form.nickname" placeholder="请输入昵称" />
      </el-form-item>

      <el-form-item label="邮箱" prop="email">
        <el-input v-model="form.email" placeholder="请输入邮箱地址" />
      </el-form-item>

      <el-form-item label="头像" prop="avatar">
        <el-input v-model="form.avatar" placeholder="请输入头像URL" />
        <div class="form-tip">支持网络图片链接</div>
      </el-form-item>

      <el-form-item label="个人简介" prop="bio">
        <el-input
          v-model="form.bio"
          type="textarea"
          :rows="4"
          placeholder="请输入个人简介"
          show-word-limit
          maxlength="200"
        />
      </el-form-item>

      <el-form-item>
        <el-button type="primary" :loading="loading" @click="handleSubmit">
          保存修改
        </el-button>
        <el-button @click="resetForm">重置</el-button>
      </el-form-item>
    </el-form>
  </el-card>
</template>

<script setup>
import { ref, reactive, watch } from 'vue'
import { useAuthStore } from '../stores/auth'
import { updateProfile } from '../api/request'
import { ElMessage } from 'element-plus'

const authStore = useAuthStore()
const formRef = ref()
const loading = ref(false)

const form = reactive({
  nickname: '',
  email: '',
  avatar: '',
  bio: ''
})

const rules = {
  email: [
    { type: 'email', message: '请输入正确的邮箱地址', trigger: 'blur' }
  ]
}

// 初始化表单数据
const initForm = () => {
  if (authStore.user) {
    form.nickname = authStore.user.nickname || ''
    form.email = authStore.user.email || ''
    form.avatar = authStore.user.avatar || ''
    form.bio = authStore.user.bio || ''
  }
}

const handleSubmit = async () => {
  if (!formRef.value) return

  try {
    const valid = await formRef.value.validate()
    if (!valid) return

    loading.value = true
    const response = await updateProfile(form)
    
    if (response.data.code === 0) {
      ElMessage.success('资料更新成功')
      // 更新 store 中的用户信息
      await authStore.fetchUserInfo()
    } else {
      ElMessage.error('更新失败')
    }
  } catch (error) {
    ElMessage.error('更新失败，请重试')
  } finally {
    loading.value = false
  }
}

const resetForm = () => {
  initForm()
  if (formRef.value) {
    formRef.value.clearValidate()
  }
}

// 监听用户信息变化，更新表单
watch(() => authStore.user, () => {
  initForm()
}, { immediate: true, deep: true })
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

:deep(.el-form-item__label) {
  font-weight: 500;
}
</style>