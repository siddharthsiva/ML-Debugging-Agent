# 🧠 Virtual-Me: Personal Behavior Analytics Platform

> **A sophisticated full-stack application that tracks, analyzes, and visualizes personal behavior patterns across multiple data sources for enhanced productivity insights.**

[![Python](https://img.shields.io/badge/Python-3.11+-blue.svg)](https://python.org)
[![React](https://img.shields.io/badge/React-18+-61DAFB.svg)](https://reactjs.org)
[![TypeScript](https://img.shields.io/badge/TypeScript-5.0+-3178C6.svg)](https://typescriptlang.org)
[![FastAPI](https://img.shields.io/badge/FastAPI-0.100+-009688.svg)](https://fastapi.tiangolo.com)
[![Docker](https://img.shields.io/badge/Docker-Ready-2496ED.svg)](https://docker.com)
[![AWS](https://img.shields.io/badge/AWS-ECS%20Ready-FF9900.svg)](https://aws.amazon.com)

## 🎯 Project Overview

**Virtual-Me** is a comprehensive personal behavior analytics platform that demonstrates modern full-stack development practices. It intelligently tracks events from various sources (GitHub, Calendar, Music, Fitness) and provides actionable insights through beautiful, interactive data visualizations.

### ✨ Key Features

🔹 **Real-time Event Timeline** - Live activity feed with source categorization  
🔹 **Focus Score Analytics** - AI-calculated productivity metrics (0-100 scale)  
🔹 **Peak Hours Heatmap** - 24-hour activity pattern visualization  
🔹 **Source Distribution Analysis** - Interactive pie charts and breakdowns  
🔹 **Intelligent Demo Data** - Realistic 7-day behavioral simulation  
🔹 **Production-Ready Architecture** - Cloud-native design with Docker & AWS

## 🏗️ Architecture & Tech Stack

### Backend
- **FastAPI** - High-performance async Python web framework
- **SQLAlchemy** - Advanced ORM with PostgreSQL/SQLite support  
- **Pydantic** - Data validation and serialization
- **Docker** - Containerization for consistent deployment
- **CORS Middleware** - Secure cross-origin resource sharing

### Frontend  
- **React 18** - Modern component-based UI library
- **TypeScript** - Type-safe development experience
- **Recharts** - Interactive data visualization library
- **Lucide React** - Beautiful, consistent icon system
- **Responsive CSS** - Mobile-first design approach

### Infrastructure
- **Docker Compose** - Multi-container orchestration
- **AWS ECS Fargate** - Serverless container deployment
- **Amazon ECR** - Container image registry
- **RDS PostgreSQL** - Managed database service

## 🚀 Quick Start

### Prerequisites
- **Docker Desktop** (recommended) or Node.js 18+ & Python 3.11+
- **Git** for cloning the repository

### 🐳 Docker Setup (Recommended)

```bash
# Clone the repository
git clone https://github.com/siddharthsiva/Virtual-Me.git
cd Virtual-Me

# Start all services
docker-compose up --build

# Seed with demo data
curl -X POST http://localhost:8000/demo/seed-data
```

### 🛠️ Manual Setup

#### Backend Setup
```bash
# Install Python dependencies
pip install -r requirements.txt

# Start the FastAPI server
uvicorn app.main:app --reload --host 0.0.0.0 --port 8000
```

#### Frontend Setup
```bash
# Navigate to frontend directory
cd frontend

# Install Node.js dependencies
npm install

# Start React development server
npm start
```

#### Seed Demo Data
```bash
# PowerShell
Invoke-RestMethod -Uri "http://localhost:8000/demo/seed-data" -Method POST

# curl (Linux/Mac)
curl -X POST http://localhost:8000/demo/seed-data
```

## 🌐 Access Points

| Service | URL | Description |
|---------|-----|-------------|
| **Dashboard** | http://localhost:3000 | Interactive analytics dashboard |
| **API Docs** | http://localhost:8000/docs | OpenAPI documentation |
| **Health Check** | http://localhost:8000/ | API status endpoint |

## 📊 Dashboard Features

### 1. **Event Timeline**
- Real-time activity feed showing recent behavioral events
- Source categorization with emoji indicators (👨‍💻 GitHub, 📅 Calendar, 🎵 Music, 💪 Fitness)
- Timestamp tracking with relative time display
- Rich metadata display for event context

### 2. **Focus Score Gauge**  
- Circular progress visualization (0-100 scale)
- Performance categorization (Excellent/Good/Fair/Needs Work)
- Smooth animation effects for engaging UX
- Algorithm-based calculation from activity patterns

### 3. **Peak Hours Heatmap**
- 24-hour activity intensity visualization
- Color-coded activity levels with hover details
- Peak productivity hours highlighting
- Daily statistics and patterns analysis

### 4. **Event Source Distribution**
- Interactive pie chart with percentage breakdowns
- Comprehensive legend with event counts
- Visual source identification with emojis
- Total statistics and active sources tracking

### 5. **Statistics Overview**
- Key metrics dashboard cards
- Total events, focus score, active sources
- Peak hours identification
- Real-time data updates

## 📡 API Documentation

### Events Endpoints
```http
GET    /events/?limit=100      # List recent events
POST   /events/                # Create new event
```

### Analytics Endpoints  
```http
GET    /analytics/summary      # Behavior summary with focus score
```

### Demo Data Endpoints
```http
POST   /demo/seed-data         # Generate 60+ realistic demo events  
GET    /demo/stats             # Database statistics
```

### Health & Documentation
```http
GET    /                       # API health check
GET    /docs                   # Interactive OpenAPI documentation
GET    /redoc                  # Alternative API documentation
```

## 🗄️ Data Models

### Event Schema
```python
class Event:
    id: int                           # Unique identifier
    source: EventSource               # github, calendar, music, fitness, other
    timestamp: datetime               # Event occurrence time
    label: str                        # Human-readable description
    event_metadata: dict              # Flexible metadata storage
```

### Analytics Schema
```python
class BehaviorSummary:
    total_events: int                 # Total event count
    events_by_source: dict[str, int]  # Source distribution
    peak_hours: list[int]             # Peak activity hours
    focus_score: float                # Calculated focus metric (0-100)
```

## 🌩️ Cloud Deployment (AWS)

### ECR Repository Setup
```bash
# Create ECR repository
aws ecr create-repository --repository-name virtual-me --region us-west-2

# Get login credentials
aws ecr get-login-password --region us-west-2 | \
  docker login --username AWS --password-stdin <account-id>.dkr.ecr.us-west-2.amazonaws.com

# Tag and push image
docker tag virtual-me:latest <account-id>.dkr.ecr.us-west-2.amazonaws.com/virtual-me:latest
docker push <account-id>.dkr.ecr.us-west-2.amazonaws.com/virtual-me:latest
```

### ECS Fargate Configuration
- **CPU**: 0.25 vCPU (scalable)
- **Memory**: 512 MB (scalable)  
- **Port**: 8000
- **Health Checks**: Enabled
- **Auto Scaling**: Configured

### Environment Variables
```bash
DATABASE_URL=postgresql://username:password@rds-endpoint:5432/virtualme
DEBUG=false
SECRET_KEY=your-production-secret-key
```

## 🧪 Development & Testing

### Code Quality
- **Type Safety**: Full TypeScript implementation
- **API Validation**: Pydantic schemas for request/response validation
- **Error Handling**: Comprehensive error responses
- **CORS Security**: Configured for production safety

### Performance Features
- **Async/Await**: Non-blocking request handling
- **Connection Pooling**: Efficient database connections
- **Caching Ready**: Structured for Redis integration
- **Pagination**: Scalable data retrieval

## 📈 Demo Data Details

The application includes intelligent demo data generation featuring:

- **GitHub Events**: Commits, PR merges, code reviews, repository activities
- **Calendar Events**: Meetings, appointments, scheduled sessions
- **Music Events**: Focus playlists, concentration sessions, productivity music  
- **Fitness Events**: Workouts, runs, health tracking, activity logs
- **Realistic Patterns**: Work hours, weekend variations, productivity cycles
- **Metadata Rich**: Detailed event information for comprehensive analytics

## 🎯 Technical Highlights

### Full-Stack Expertise
✅ **Modern Python Backend** - FastAPI with async/await patterns  
✅ **React + TypeScript Frontend** - Type-safe component architecture  
✅ **Interactive Data Visualization** - Professional charts and animations  
✅ **Database Design** - Normalized schema with flexible metadata  
✅ **API Development** - RESTful design with OpenAPI documentation  
✅ **Containerization** - Docker multi-stage builds and optimization

### Production Readiness
✅ **Cloud-Native Architecture** - AWS ECS Fargate deployment ready  
✅ **Security Implementation** - CORS, input validation, error handling  
✅ **Monitoring Preparation** - Health checks and structured logging  
✅ **Scalability Design** - Microservices patterns and database optimization  
✅ **DevOps Practices** - CI/CD ready with containerization

## 🔮 Future Enhancements

### Phase 2 Features
- [ ] **Machine Learning Models** - Behavior prediction algorithms
- [ ] **Mobile Application** - React Native companion app
- [ ] **Team Analytics** - Collaborative insights and comparisons  
- [ ] **API Integrations** - Slack, Jira, Spotify, Google Calendar
- [ ] **Real-time Updates** - WebSocket implementation for live data
- [ ] **Advanced Visualizations** - Cohort analysis, trend forecasting

### Scalability Improvements
- [ ] **Microservices Architecture** - Domain-driven service separation
- [ ] **Event Streaming** - Apache Kafka for real-time data processing
- [ ] **Caching Layer** - Redis for performance optimization
- [ ] **CDN Integration** - Global content delivery
- [ ] **Advanced Monitoring** - APM with comprehensive observability

## 📝 Contributing

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/amazing-feature`
3. Commit changes: `git commit -m 'Add amazing feature'`
4. Push to branch: `git push origin feature/amazing-feature`
5. Open a Pull Request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 👨‍💻 Author

**Siddharth Siva** - Full-Stack Engineer  
- 🔗 [GitHub](https://github.com/siddharthsiva)
- 📧 Contact via GitHub

---

*Built with ❤️ using FastAPI, React, and modern cloud technologies for demonstrating enterprise-level full-stack development capabilities.*