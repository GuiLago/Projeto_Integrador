// [ PROJETO INTEGRADOR - SENAI ]

// ----------------- Motores (L298N) ---------------------------
#define IN1 5
#define IN2 6
#define IN3 10
#define IN4 11

// Velocidades otimizadas
int vel = 180;             // Velocidade de busca
const int velAtaque = 255; // Velocidade máxima no ataque
const int velGiro = 200;   // Velocidade de giro
const int velRe = 220;     // Velocidade de ré

// -------------- Sensores de Linha ----------------------------
#define ESQ A0
#define DIR A1
#define TRAS A2

const int limiarBranco = 400; // Ajuste conforme calibração

// -------------- Ultrassônicos --------------------------------
#define TRIG_F 7
#define ECHO_F 8
#define TRIG_E 2
#define ECHO_E 4
#define TRIG_D 13
#define ECHO_D 3

// Distâncias otimizadas para arena de 80cm (valores em CM)
const int distAtaque = 40;       // Distância para iniciar ataque (cm)
const int distProximo = 20;      // Oponente muito próximo (cm)
const int distMuitoProximo = 10; // Empurrar com tudo! (cm)

// -------------- Variáveis de Estratégia ----------------------
unsigned long ultimoTempoOponente = 0;
int ultimaDirecaoOponente = 0; // 0=frente, 1=esq, 2=dir
bool modoAtaque = false;
unsigned long tempoModoAtaque = 0;

// Funções de Estado e Movimentação

void frente(int velocidade = 0)
{
    if (velocidade == 0)
        velocidade = vel;
    analogWrite(IN1, velocidade);
    analogWrite(IN2, 0);
    analogWrite(IN3, velocidade);
    analogWrite(IN4, 0);
}

void tras(int velocidade = 0)
{
    if (velocidade == 0)
        velocidade = velRe;
    analogWrite(IN1, 0);
    analogWrite(IN2, velocidade);
    analogWrite(IN3, 0);
    analogWrite(IN4, velocidade);
}

void esquerda(int velocidade = 0)
{
    if (velocidade == 0)
        velocidade = velGiro;
    analogWrite(IN1, 0);
    analogWrite(IN2, velocidade);
    analogWrite(IN3, velocidade);
    analogWrite(IN4, 0);
}

void direita(int velocidade = 0)
{
    if (velocidade == 0)
        velocidade = velGiro;
    analogWrite(IN1, velocidade);
    analogWrite(IN2, 0);
    analogWrite(IN3, 0);
    analogWrite(IN4, velocidade);
}

void curvaEsquerda(int velocidade = 0)
{
    if (velocidade == 0)
        velocidade = vel;
    analogWrite(IN1, velocidade / 3);
    analogWrite(IN2, 0);
    analogWrite(IN3, velocidade);
    analogWrite(IN4, 0);
}

void curvaDireita(int velocidade = 0)
{
    if (velocidade == 0)
        velocidade = vel;
    analogWrite(IN1, velocidade);
    analogWrite(IN2, 0);
    analogWrite(IN3, velocidade / 3);
    analogWrite(IN4, 0);
}

void parar()
{
    analogWrite(IN1, 0);
    analogWrite(IN2, 0);
    analogWrite(IN3, 0);
    analogWrite(IN4, 0);
}

// Funções de Medição

long medirDist(int trig, int echo)
{
    digitalWrite(trig, LOW);
    delayMicroseconds(2);
    digitalWrite(trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig, LOW);

    long dur = pulseIn(echo, HIGH, 30000);
    if (dur == 0)
        return 999; // Timeout = sem obstáculo

    long dist = (dur * 0.0343) / 2; // Retorna em CM
    return (dist > 0 && dist < 400) ? dist : 999;
}

bool detectarBorda()
{
    int sEsq = analogRead(ESQ);
    int sDir = analogRead(DIR);
    int sTras = analogRead(TRAS);

    bool bordaEsq = (sEsq < limiarBranco);
    bool bordaDir = (sDir < limiarBranco);
    bool bordaTras = (sTras < limiarBranco);

    // Lógica de escape aprimorada
    if (bordaEsq && bordaDir)
    {
        // Ambos sensores frontais na borda - recuar e girar
        tras(velRe);
        delay(300);
        direita(velGiro);
        delay(250);
        return true;
    }
    else if (bordaEsq)
    {
        // Borda à esquerda - recuar e virar direita
        tras(velRe);
        delay(200);
        direita(velGiro);
        delay(200);
        return true;
    }
    else if (bordaDir)
    {
        // Borda à direita - recuar e virar esquerda
        tras(velRe);
        delay(200);
        esquerda(velGiro);
        delay(200);
        return true;
    }
    else if (bordaTras)
    {
        // Borda atrás - avançar rápido
        frente(velAtaque);
        delay(300);
        return true;
    }

    return false;
}

void estrategiaAtaque(long distF, long distE, long distD)
{

    // Encontrar menor distância e direção
    long menorDist = min(min(distF, distE), distD);

    // Oponente detectado
    if (menorDist < distAtaque)
    {

        modoAtaque = true;
        tempoModoAtaque = millis();
        ultimoTempoOponente = millis();

        // PRIORIDADE: Frente (ataque direto)
        if (distF < distAtaque && distF <= distE && distF <= distD)
        {
            ultimaDirecaoOponente = 0;

            if (distF < distMuitoProximo)
            {
                // CONTATO! Empurra com tudo!
                frente(velAtaque);
            }
            else if (distF < distProximo)
            {
                // Oponente muito próximo - força máxima!
                frente(velAtaque);
            }
            else
            {
                // Aproximando - velocidade alta
                frente(velAtaque - 30);
            }
        }
        // Oponente à ESQUERDA
        else if (distE < distAtaque && distE < distF)
        {
            ultimaDirecaoOponente = 1;

            if (distE < distProximo)
            {
                // Muito próximo - curva agressiva
                curvaEsquerda(velAtaque);
            }
            else
            {
                // Girar para interceptar
                esquerda(velGiro);
            }
        }
        // Oponente à DIREITA
        else if (distD < distAtaque && distD < distF)
        {
            ultimaDirecaoOponente = 2;

            if (distD < distProximo)
            {
                // Muito próximo - curva agressiva
                curvaDireita(velAtaque);
            }
            else
            {
                // Girar para interceptar
                direita(velGiro);
            }
        }
    }
    // Nenhum oponente detectado
    else
    {
        estrategiaBusca();
    }
}

void estrategiaBusca()
{

    // Se viu oponente recentemente (< 2 seg), girar na última direção
    if (millis() - ultimoTempoOponente < 2000)
    {

        switch (ultimaDirecaoOponente)
        {
        case 0: // Estava na frente - avançar um pouco
            frente(vel);
            break;
        case 1: // Estava à esquerda
            esquerda(velGiro);
            break;
        case 2: // Estava à direita
            direita(velGiro);
            break;
        }
    }
    // Busca padrão - movimento espiral (otimizado para arena 80cm)
    else
    {
        // Alterna entre avançar e girar (rápido em arena pequena)
        if ((millis() / 800) % 2 == 0)
        {
            frente(vel);
        }
        else
        {
            direita(velGiro); // Gira criando espiral
        }
    }
}

void setup()
{
    Serial.begin(9600);

    // Configurar pinos dos motores
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);

    // Configurar sensores de linha
    pinMode(ESQ, INPUT);
    pinMode(DIR, INPUT);
    pinMode(TRAS, INPUT);

    // Configurar ultrassônicos
    pinMode(TRIG_F, OUTPUT);
    pinMode(ECHO_F, INPUT);
    pinMode(TRIG_E, OUTPUT);
    pinMode(ECHO_E, INPUT);
    pinMode(TRIG_D, OUTPUT);
    pinMode(ECHO_D, INPUT);

    parar();

    // Delay inicial (regra do sumô - 5 segundos)
    Serial.println("=== ROBÔ SUMÔ INICIANDO ===");
    delay(2000);
    Serial.println("LUTA!");
}

void loop()
{

    // PRIORIDADE 1: Evitar cair do ringue
    if (detectarBorda())
    {
        return; // Volta ao início do loop
    }

    // Ler sensores ultrassônicos
    long distF = medirDist(TRIG_F, ECHO_F);
    long distE = medirDist(TRIG_E, ECHO_E);
    long distD = medirDist(TRIG_D, ECHO_D);

    // Debug
    Serial.print("F:");
    Serial.print(distF);
    Serial.print("cm");
    Serial.print(" E:");
    Serial.print(distE);
    Serial.print("cm");
    Serial.print(" D:");
    Serial.print(distD);
    Serial.print("cm");
    Serial.print(" | Modo: ");
    Serial.println(modoAtaque ? "ATAQUE" : "BUSCA");

    // PRIORIDADE 2: Executar estratégia
    estrategiaAtaque(distF, distE, distD);

    delay(10); // Pequeno delay para estabilidade
}
