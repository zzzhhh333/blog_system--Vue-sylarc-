<template>
  <el-dialog
    v-model="visible"
    :title="blog ? '编辑博客' : '创建博客'"
    width="800px"
    :before-close="handleClose"
  >
    <el-form :model="form" :rules="rules" ref="formRef" label-width="80px">
      <el-form-item label="标题" prop="title">
        <el-input v-model="form.title" placeholder="请输入博客标题" />
      </el-form-item>
      
      <el-form-item label="摘要" prop="summary">
        <el-input
          v-model="form.summary"
          type="textarea"
          :rows="3"
          placeholder="请输入博客摘要（可选）"
          show-word-limit
          maxlength="200"
        />
      </el-form-item>
      
      <el-form-item label="内容" prop="content">
        <el-input
          v-model="form.content"
          type="textarea"
          :rows="10"
          placeholder="请输入博客内容"
          show-word-limit
          maxlength="10000"
        />
      </el-form-item>
      
      <el-form-item label="状态" prop="status">
        <el-radio-group v-model="form.status">
          <el-radio :label="0">草稿</el-radio>
          <el-radio :label="1">发布</el-radio>
        </el-radio-group>
      </el-form-item>
    </el-form>
    
    <template #footer>
      <el-button @click="handleClose">取消</el-button>
      <el-button type="primary" :loading="loading" @click="handleSubmit">
        {{ blog ? '更新' : '发布' }}
      </el-button>
    </template>
  </el-dialog>
</template>

<script setup>
import { ref, reactive, watch, nextTick } from 'vue'
import { createBlog, updateBlog } from '../api/request'
import { ElMessage } from 'element-plus'

const props = defineProps({
  modelValue: Boolean,
  blog: Object
})

const emit = defineEmits(['update:modelValue', 'success'])

const visible = ref(false)
const formRef = ref()
const loading = ref(false)

const form = reactive({
  title: '',
  summary: '',
  content: '',
  status: 1
})

const rules = {
  title: [
    { required: true, message: '请输入标题', trigger: 'blur' }
  ],
  content: [
    { required: true, message: '请输入内容', trigger: 'blur' }
  ]
}

watch(() => props.modelValue, (val) => {
  visible.value = val
  if (val && props.blog) {
    Object.assign(form, props.blog)
  } else if (val) {
    resetForm()
  }
})

watch(visible, (val) => {
  emit('update:modelValue', val)
})

const resetForm = () => {
  Object.assign(form, {
    title: '',
    summary: '',
    content: '',
    status: 1
  })
  nextTick(() => {
    if (formRef.value) {
      formRef.value.clearValidate()
    }
  })
}

const handleClose = () => {
  visible.value = false
  resetForm()
}

const handleSubmit = async () => {
  if (!formRef.value) return
  
  try {
    const valid = await formRef.value.validate()
    if (!valid) return
    
    loading.value = true
    let response
    
    if (props.blog) {
      response = await updateBlog({
        id: props.blog.id,
        ...form
      })
    } else {
      response = await createBlog(form)
    }
    
    if (response.data.code === 200) {
      ElMessage.success(props.blog ? '更新成功' : '发布成功')
      emit('success')
      handleClose()
    } else {
      ElMessage.error('操作失败')
    }
  } catch (error) {
    ElMessage.error('操作失败，请重试')
  } finally {
    loading.value = false
  }
}
</script>