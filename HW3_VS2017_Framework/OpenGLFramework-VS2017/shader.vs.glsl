#version 330

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec2 aTexCoord;

out vec3 vertex_color;
out vec2 texCoord;
out vec3 Normal;
out vec3 FragPos;
out vec3 LightPos;

uniform mat4 um4p;
uniform mat4 um4v;
uniform mat4 um4m;

// ------------customize vars--------------

uniform int ShadingType; //0=gouraud 1=phong
uniform int LightMode;   // 0=Directional light  1=Position(point) light 2=Spot light
uniform vec3 viewPos;


uniform int isEye;

struct Offset
{
    float x;
    float y;
};
uniform Offset eyeOffset;

struct Light
{
    float angle;
    vec3 position;
    vec3 diffuse;
};
uniform Light light;

struct Material
{
    vec3 Ka;
    vec3 Kd;
    vec3 Ks;
    float shininess;
};
uniform Material material;

// ---for attenuation---
vec3 result;
float constant;
float linear;
float quadratic;
float distance;
float attenuation;
float theta;
// ------------customize end-------------

// [TODO] passing uniform variable for texture coordinate offset

void main()
{
    // [TODO]
    if(isEye==1){
        texCoord = vec2(aTexCoord.x+eyeOffset.x , aTexCoord.y+eyeOffset.y );
    }else{
        texCoord = vec2(aTexCoord.x , aTexCoord.y );

    }
    vertex_color = aColor;
    gl_Position = um4p * um4v * um4m * vec4(aPos, 1.0);
    FragPos = vec3(um4v * um4m * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(um4v * um4m))) * aNormal;
    LightPos = vec3(um4v * vec4(light.position, 1.0));

    if (ShadingType == 0)
    {
        // gouraud shading
        // ------------------------
        if (LightMode == 0)
        {
            // ambient
            vec3 ambient = vec3(0.15f, 0.15f, 0.15f) * material.Ka;
            // diffuse
            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(vec3(1.0f, 1.0f, 1.0f));
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = light.diffuse * diff * material.Kd ;
            // specular
            vec3 lightspecular = light.diffuse;
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 halfwayDir = normalize(lightDir + viewDir);
            float spec = pow(max(dot(norm, halfwayDir), 0.0), material.shininess);
            vec3 specular = lightspecular * (spec * material.Ks);
            result = ambient + diffuse + specular;
            vertex_color = result;
        }
        if (LightMode == 1)
        {
            // ambient
            vec3 ambient = vec3(0.15f, 0.15f, 0.15f) * material.Ka;

            // diffuse
            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(LightPos - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = light.diffuse * material.Kd * diff;

            // specular
            vec3 lightspecular = light.diffuse;
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 halfwayDir = normalize(lightDir + viewDir);
            float spec = pow(max(dot(norm, halfwayDir), 0.0), material.shininess);
            vec3 specular = lightspecular * (spec * material.Ks);

            //attenuation
            constant = 0.01;
            linear = 0.8;
            quadratic = 0.1;
            distance = length(LightPos - FragPos);
            attenuation = min(1.0f / (constant + linear * distance + quadratic * (distance * distance)), 1);
            result = ambient * attenuation + diffuse * attenuation + specular * attenuation;
            vertex_color = result;
        }
        if (LightMode == 2)
        {
            // ambient
            vec3 ambient = vec3(0.15f, 0.15f, 0.15f) * material.Ka;

            // diffuse
            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(LightPos - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = light.diffuse * material.Kd * diff;

            // specular
            vec3 lightspecular = light.diffuse;
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 halfwayDir = normalize(lightDir + viewDir);
            float spec = pow(max(dot(norm, halfwayDir), 0.0), material.shininess);
            vec3 specular = lightspecular * (spec * material.Ks);

            //attenuation
            constant = 0.05;
            linear = 0.3;
            quadratic = 0.6;
            distance = length(LightPos - FragPos);
            theta = dot(lightDir, normalize(-vec3(0, 0, -1)));
            float spoteffect = pow(max(theta, 0), 50);

            if (theta > cos(radians(light.angle)))
            {
                attenuation = min(1.0f / (constant + linear * distance + quadratic * (distance * distance)), 1);
                result = spoteffect * (ambient * attenuation + diffuse * attenuation + specular * attenuation);
            }
            else
            {
                result = spoteffect * ambient;
            }

            vertex_color = result;
        }
    }
}
