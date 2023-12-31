// search_ver3と同等のコード
// lotusフォルダの中で一番時間効率の良いやつを代表してこのファイルに記入

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <limits.h>

// 同じ列のどこに同じ値を持つ行があるか、位置把握用の構造体
typedef struct
{
  int m;             // 同じ値を持つ行の総数
  int vec[2][10000]; // 同じ値を持つものの座標。vec[0][~]はx座標でvec[1][~]はy座標。
} SameValue;

// 回答出力のためのパラメータ構造体
typedef struct
{
  int start;
  int length;
  int match_count;
} ANSWER;

// 各start、lengthにおけるansを保存
typedef struct
{
  ANSWER ans[10000][300];
  int m[10000][300];
} MatchValue;

// データの読み込み
char *
read_file(char *fileName)
{
  FILE *fp = fopen(fileName, "r");
  if (fp == NULL)
  {
    perror("ファイルを開く際にエラーが発生しました");
    return NULL;
  }

  fseek(fp, 0, SEEK_END);     // メモリポインタを最後に移動
  long int fsize = ftell(fp); // ファイルサイズを取得
  rewind(fp);                 // メモリポインタを先頭に戻す

  // ファイルサイズ分のメモリを確保（改行+ヌル終端文字も含む）
  char *strBuffer = (char *)malloc(fsize + 2);
  if (strBuffer == NULL)
  {
    perror("メモリ確保に失敗しました");
    fclose(fp);
    return NULL;
  }

  size_t readCount = fread(strBuffer, sizeof(char), fsize, fp);
  if (strBuffer[readCount - 1] != '\n')
  {
    strBuffer[readCount++] = '\n'; // 改行で終わっていない場合は改行を足す(後から使う)
  }
  strBuffer[readCount] = '\0'; // 文字列をヌル終端

  fclose(fp);

  // printf("%s", strBuffer); // 内容を出力
  return strBuffer;
}

void count_have_same_value(int M, int N, char *strBuffer_data, SameValue **array_write)
{
  int m;
  int pos_A;
  int pos_other;
  // 一行目が基準データ(Aさんのデータ)になる
  // printf("start:%d, cols:%d, rows:%d\n", start, cols, rows);
  for (int i = 0; i < M; i++)
  {
    for (int j = 0; j < N; j++)
    {
      pos_A = i;
      pos_other = i + j * M;
      m = array_write[i]->m;
      // if (thread_param->array[pos_other] == '\n')
      // {
      //     printf("T:%d, \\, %d\n", thread_param->thread_num, pos_other);
      // }
      // else
      // {
      //     printf("T:%d, %c, %d\n", thread_param->thread_num, thread_param->array[pos_other], pos_other);
      // }

      // もしかしたらアクセスが遅いかもしれない。要検討。
      // pos_otherがpos_Aと同じ行にない状態で値が一致するもののみを抽出してカウント
      if (((int)strBuffer_data[pos_other] == strBuffer_data[pos_A]) && (pos_A != pos_other)) // 条件:回答一致
      {
        array_write[i]->vec[0][m] = pos_A; // x
        array_write[i]->vec[1][m] = j;     // y
        array_write[i]->m += 1;
      }
    }
  }
}

// 範囲を調べて一致する人数を取得する
ANSWER analyzeRange(char *model, SameValue **same_value_array, char *strBuffer_data, int M, MatchValue *match_value)
{
  // 情報が文字列として与えられるので、分解して開始点と長さを取得
  char *saveptr; // 分離後の文字列を表すポインタ
  ANSWER ans;
  const char *delimiter = ",";
  int start, length;
  int m, min = INT_MAX, min_pos = 0;
  int x, y, x_min, y_min;
  int val_A, val_other, row_other;
  int flg = 1;

  int m_nearest_length = 0;

  start = atoi(strtok_r(model, delimiter, &saveptr));
  length = atoi(strtok_r(NULL, delimiter, &saveptr));
  ans.start = start;
  ans.length = length;
  ans.match_count = 0;
  // printf("start:%d, length:%d\n", start, length);

  if (match_value->ans[start][length].start == 0) // 既に同じstart、lengthで実施しているかで分岐
  {
    // 次のfor文で走査する範囲を限定するためにmを再使用
    for (int k = length; k >= 0; k--)
    {
      if (match_value->m[start][k] != 0)
      {
        m_nearest_length = k;
        break;
      }
    }

    // same_value_arrayのmの値を捜査して最小の値を見つける
    for (int i = start + m_nearest_length; i < start + length; i++)
    {
      m = same_value_array[i]->m;
      if (min > m)
      {
        min = m;
        min_pos = i;
      }
    }
    // printf("min pos:%d\n", min_pos);

    match_value->m[start][length] = min_pos;

    // 見つかった最小のmの値をもとに、その位置の要素のx座標の値が範囲の中のx座標に含まれるかどうかを調べる
    for (int i = 0; i < same_value_array[min_pos]->m; i++)
    { // 1回目のループは見つかった最小のmに対してそのmの項目を全部を調べるために行う
      flg = 1;
      y = same_value_array[min_pos]->vec[1][i];
      row_other = y * M;
      for (int j = start; j < start + length; j++)
      {
        // 同じy列のものについて調べる
        val_A = strBuffer_data[j];
        val_other = strBuffer_data[row_other + j];
        if (val_A != val_other)
        {
          flg = 0;
          break;
        }
      }

      if (flg == 1) // 列範囲と一致した列の数が同じとき完全一致の人がいる
      {
        ans.match_count++;
      }
    }

    match_value->ans[start][length] = ans;
    return ans;
  }
  else // 既に同じstart、lengthで実施している場合
  {
    return match_value->ans[start][length];
  }
}

void save_to_file(char *fileName, char *data)
{
  FILE *fp;
  fp = fopen(fileName, "w");
  fprintf(fp, "%s\n", data);
  fclose(fp);
}

int main(int argc, char *argv[])
{
  int start_time, end_time;
  start_time = clock();
  //**************************************************dataの読み込み*************************************************************
  // char *files_data[] = {"data0"}; // 読み込むファイル群
  // char *files_range[] = {"range0"};
  // char *files_save[] = {"answer00"};

  char *strBuffer_data = read_file(argv[1]);
  char *strBuffer_range = read_file(argv[2]);

  // 二次元配列に分割せずそのまま使用(分割する手間を省く)
  // アクセスは以下のようにしてstrBuffer[i * M + j]で二次元配列的に扱うことができる。ただし、この方法が早いかはわからない。
  // この方法を採用したのは分割するコストが高いと思ったからであり、読み込みに使った配列をそのまま使えばメモリ効率が良いのではないかと考えた。
  // わんちゃんキャッシュに乗り切る可能性も考えて一次元配列にした。
  //
  //  for (int i = 0; i < N; i++)
  //  {
  //      for (int j = 0; j < M; j++)
  //      {
  //          printf("%c", strBuffer_data[i * (M + 1) + j]);
  //      }
  //      printf("\n");
  //  }

  // 最初にM(アンケートの個数)を取得するために改行位置を一つだけ調べる

  char *chL_pos = strchr(strBuffer_data, '\n'); // 改行位置のアドレス

  int M = (int)(chL_pos - strBuffer_data) + 1; // アンケートの質問数(改行列含む)を取得
  int N = strlen(strBuffer_data) / M;          // 回答人数を取得
  printf("\nM: %d N:%d\n", M, N);

  //*********************************************************Aと同じものを事前に調べておく*************************************************
  // 構造体内の動的配列用にメモリを確保
  SameValue **same_value_array = malloc(sizeof(SameValue *) * M);
  for (int i = 0; i < M; i++)
  {
    same_value_array[i] = malloc(sizeof(SameValue));
  }

  // 以下Dataの読み込みとAと同じ回答をした数を列ごとにカウントする処理
  count_have_same_value(M, N, strBuffer_data, same_value_array);

  // 事前に調べておいた一致する行の数を表示
  // printf("Totaling:");
  // for (int i = 0; i < M; i++)
  // {
  //     printf("%d", same_value_array[i]->m);
  // }
  // printf("\n");

  // 事前に調べておいた一致する行を表示
  // for (int i = 0; i < M; i++)
  // {
  //     printf("m:%d ", i);
  //     for (int j = 0; j < same_value_array[i]->m; j++)
  //     {
  //         printf("(%d, %d)", same_value_array[i]->vec[0][j], same_value_array[i]->vec[1][j]);
  //     }
  //     printf("\n");
  // }

  //**************************************************************rangeの読み込み******************************************************
  // printf("%s\n", strBuffer_range);
  ANSWER ans;
  int loop_count = 1;
  const char *delimiter = "\n";
  char *line;
  char *model_name;
  char *saveptr;
  char *save_data = (char *)malloc(INT_MAX);
  if (save_data == NULL)
  {
    perror("メモリを確保に失敗しました");
    return 1;
  }

  // ans保持用構造体の初期化
  MatchValue *match_value = (MatchValue *)malloc(sizeof(MatchValue));
  if (match_value == NULL)
  {
    perror("メモリを確保に失敗しました");
    return 1;
  }

  model_name = strtok_r(strBuffer_range, delimiter, &saveptr); // モデル名
  line = strtok_r(NULL, delimiter, &saveptr);                  // 一行目

  while (line != NULL) // 二行目以降
  {
    ans = analyzeRange(line, same_value_array, strBuffer_data, M, match_value);              // 答えを取得
    sprintf(save_data, "%s%d, %d, %d\n", save_data, ans.start, ans.length, ans.match_count); // 書き込み用の変数に格納
    // printf("match:%d\n", ans.match_count);

    // printf("strBuffer_range:%s\n", strBuffer_range);
    // 次の行を取得
    line = strtok_r(NULL, delimiter, &saveptr); // 次の行データを取得
    loop_count++;
  }

  //****************************************************************結果の出力***********************************************************
  save_data[strlen(save_data) - 1] = '\0'; // 末尾の改行を削除
  save_to_file(argv[3], save_data);        // 結果をファイルに保存
  // printf("%s\n", save_data);
  printf("total count:%d\n", loop_count); // ループの総回数を表示

  //****************************************************************解放処理**************************************************************
  for (int i = 0; i < M; i++)
  {
    free(same_value_array[i]);
  }
  free(strBuffer_data);
  free(same_value_array);
  free(save_data);
  free(match_value);

  end_time = clock();
  printf("time:%f\n", (double)(end_time - start_time) / CLOCKS_PER_SEC);

  return 0;
}