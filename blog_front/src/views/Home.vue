<template>
  <div class="home">
    <div class="hero-section">
      <h1>欢迎来到博客系统</h1>
      <p>分享你的想法，发现精彩内容</p>
    </div>
    
    <div class="blog-list-container">
      <h2>最新博客</h2>
      <BlogList />
    </div>
  </div>
</template>

<script setup>
import BlogList from '../components/BlogList.vue'
</script>

<style scoped>
.hero-section {
  text-align: center;
  padding: 60px 0;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
  margin-bottom: 40px;
}

.hero-section h1 {
  font-size: 3em;
  margin-bottom: 20px;
}

.hero-section p {
  font-size: 1.2em;
  opacity: 0.9;
}

.blog-list-container {
  max-width: 1200px;
  margin: 0 auto;
  padding: 0 20px;
}
</style>