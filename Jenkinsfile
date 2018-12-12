pipeline {
    agent any

    stages {
        stage ('Cleanup Stage') {

            steps {
                sh 'make wipe-release'
            }
        }

        stage ('Compile Stage') {

            steps {
                sh 'make build-release'
            }
        }
    }
}
