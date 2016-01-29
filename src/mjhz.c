﻿#include "mjhz.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "card_sort.h"

typedef struct analyse_r_s{
    int n1;
    int v1[GP_MAX_CARDS];
    int n2;
    int v2[GP_MAX_CARDS/2];
    int n3;
    int v3[GP_MAX_CARDS/3];
    int n4;
    int v4[GP_MAX_CARDS/4];
}analyse_r;

void mjhz_init(mjhz_t* mj, int mode, int player_num)
{
    int i,j,n;
    mj_t card;

    if(!mj)
        return;
    memset(mj, 0, sizeof(mjhz_t));

    if (mode == MJHZ_MODE_SERVER)
        mj->mode = MJHZ_MODE_SERVER;
    else
        mj->mode = MJHZ_MODE_CLIENT;
    mj->player_num = player_num;
    mj->game_state = MJHZ_GAME_END;
    mj->inning = 0;
    mj->turn_time = 30;

	/* 杭州麻将使用136张牌 */
	/* 序数牌 */
	n = 0;
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 10; j++) {
			mj->deck[n].suit = mjSuitWan;
			mj->deck[n].sign = mj1 + j;
			n++;
			mj->deck[n].suit = mjSuitSuo;
			mj->deck[n].sign = mj1 + j;
			n++;
			mj->deck[n].suit = mjSuitTong;
			mj->deck[n].sign = mj1 + j;
			n++;
		}
	}
	/* 字牌 */
	for (i = 0; i < 4; i++) {
		mj->deck[n].suit = mjSuitFeng;
		mj->deck[n].sign = mjEast;
		n++;
		mj->deck[n].suit = mjSuitFeng;
		mj->deck[n].sign = mjSouth;
		n++;
		mj->deck[n].suit = mjSuitFeng;
		mj->deck[n].sign = mjWest;
		n++;
		mj->deck[n].suit = mjSuitFeng;
		mj->deck[n].sign = mjNorth;
		n++;
		mj->deck[n].suit = mjSuitZFB;
		mj->deck[n].sign = mjZhong;
		n++;
		mj->deck[n].suit = mjSuitZFB;
		mj->deck[n].sign = mjFa;
		n++;
		mj->deck[n].suit = mjSuitZFB;
		mj->deck[n].sign = mjBai;
		n++;
	}
    mj->deck_all_num = n;

    for(i = 0; i < MJHZ_MAX_PLAYER; i++){
        mj->players[i].position = i;
    }
}

int mjhz_deal(mjhz_t* mj, mjpai_t* card)
{
    int i;
    mjpai_t* p;

    if (!mj || !card)
        return -1;

    if (mj->deck_valid_num <= 0)
        return -2;

	mj->deck_deal_index++;
	if (mj->deck_deal_index >= mj->deck_all_num) {
		mj->deck_deal_index = 0;
	}
	card->suit = mj->deck[mj->deck_deal_index].suit;
	card->sign = mj->deck[mj->deck_deal_index].sign;
    return 0;
}

void mjhz_start(mjhz_t* mj)
{
    int i,direct,m,n;
    int start_num;
    mjpai_t card;

    if(!mj)
        return;
    mj->round = 0;
    mj->game_state = GP_GAME_PLAY;
    mj->inning++;

	mj->dice1 = rand() % 6 + 1;
	mj->dice2 = rand() % 6 + 1;

	/* 白板是财神 */
	mj->mammon.suit = mjSuitZFB;
	mj->mammon.sign = mjBai;

	mj->last_played_mj.suit = 0;
	mj->last_played_mj.sign = 0;

    if (mj->mode == GP_MODE_SERVER) {
		/* 洗牌 */
		mj_shuffle(mj->deck, mj->deck_all_num);
		/* 计算起手牌位置 */
		direct = (mj->banker_no + MJHZ_MAX_PLAYERS
			- (mj->dice1 + mj->dice2) % MJHZ_MAX_PLAYERS)
			% MJHZ_MAX_PLAYERS;
		mj->deck_deal_index = direct * 17 * 2 
			+ (mj->dice1 + mj->dice2) * 2;
		mj->deck_deal_gang = mj->deck_deal_index - 1; /* 杠抓牌 */
		mj->deck_deal_end = (mj->deck_deal_index + mj->deck_all_num
				- 20) % mj->deck_all_num;
		mj->deck_valid_num = gp->deck_all_num - 20;

		/* 顺时针,每人抓12张,庄家先抓 */
		m = n = 0;
		for (i = 0; i < MJHZ_MAX_PLAYERS; i++) {
			direct = (mj->banker_no + i) % MJHZ_MAX_PLAYERS;
			memcpy(mj->players[direct].cards + m, mj->deck + n, 4 * sizeof(mjpai_t));
			n += 4;
		}
		m += 12;
		/* 庄家跳牌2张,其他人一张 */
		mj->players[banker_no].cards[m].suit = mj->deck[n].suit;
		mj->players[banker_no].cards[m].sign = mj->deck[n].sign;
		mj->players[banker_no].cards[m+1].suit = mj->deck[n+4].suit;
		mj->players[banker_no].cards[m+1].sign = mj->deck[n+4].sign;
		n++;
		for (i = 0; i < MJHZ_MAX_PLAYERS; i++) {
			if (i == banker_no) continue;
			direct = (mj->banker_no + i) % MJHZ_MAX_PLAYERS;
			mj->players[direct].cards[m].suit = mj->deck[n].suit;
			mj->players[direct].cards[m].suit = mj->deck[n].sign;
		}
        /* the first player */
        mj->first_player_no = mj->banker_no;
        mj->curr_player_no = mj->first_player_no;
    } else {
        for (i = 0; i < MJHZ_MAX_PLAYERS; i++) {
            memset(mj->players[i].cards, 0, sizeof(mjpai_t) * MJHZ_MAX_CARDS);
            memset(mj->players[i].cards_played, 0, sizeof(mjpai_t) * MJHZ_MAX_CARDS);
            mj->players[i].num_valid_card = 0;
        }
		mj->banker_no = 0;
        mj->first_player_no = 0;
        mj->curr_player_no = 0;
    }
}

void mjhz_sort(mjpai_t* cards, int len)
{
    mj_sort(cards, len);
}

const char* mjhz_hu_name(mjhz_hu_t* hu)
{
    static char* hu_name[] = {
        "MJHZ_ERROR",
        "MJHZ_PING",	/* 平胡 */
        "MJHZ_DUI7",	/* 7对子 */
        "MJHZ_LONG",	/* 一条龙 */
        "MJHZ_QYS"		/* 清一色*/
    };

    if (htype <= 4)
        return htype_name[htype];
    else
        return htype_name[0];
}

void mjhz_play(mjhz_t* mj, int player_no, mjpai_t* card)
{
}

void mjhz_draw(mjhz_t* mj, int is_gang)
{
}

void gp_handtype(gp_t* gp, card_t* cards, int len, hand_type* ht)
{
    int flag,i,n,have_k,have_3;
    analyse_r ar;
    card_t *p;
    cd_bucket x[20];

    if (!gp || !cards || len <= 0 || !ht)
        return;
    ht->type = GP_ERROR;
    p = cards;
    ht->num = cards_num(cards, len);
    switch (ht->num) {
        case 0:
            return;
        case 1:
            ht->type = GP_SINGLE;
            ht->type_card.rank = p->rank;
            ht->type_card.suit = p->suit;
            ht->param = card_logicvalue(p);
            return;
        case 2:
            if (p->rank == (p + 1)->rank) {
                ht->type = GP_DOUBLE;
                ht->type_card.rank = p->rank;
                ht->type_card.suit = p->suit;
                ht->param = card_logicvalue(p);
                return;
            }
            return;
        default:
            break;
    }

    memset(&ar, 0, sizeof(analyse_r));
    memset(x, 0, sizeof(cd_bucket) * 20);
    cards_bucket(cards, len, x);
    ar.n1 = ar.n2 = ar.n3 = ar.n4 = 0;
    for (i = 19; i >= 0; --i) {
        n = x[i].num_spade + x[i].num_heart + x[i].num_club + x[i].num_diamond;
        switch (n) {
            case 1:
                ar.v1[ar.n1] = i;
                ar.n1++;
                break;
            case 2:
                ar.v2[ar.n2] = i;
                ar.n2++;
                break;
            case 3:
                ar.v3[ar.n3] = i;
                ar.n3++;
                break;
            case 4:
                ar.v4[ar.n4] = i;
                ar.n4++;
                break;
        }
    }

    /* for bomb */
    if (ar.n4 > 0) {
        if (ar.n4 == 1 && ht->num == 4) {
            if(gp->game_rule == GP_RULE_DEFAULT)
                ht->type = GP_BOMB;
            else
                ht->type = GP_FOUR;
            ht->type_card.rank = x[ar.v4[0]].rank;
            ht->type_card.suit = get_bucket_suit(&x[ar.v3[0]]);
            ht->param = ar.v4[0];
            return;
        }
        if (ar.n4 == 1 && ht->num == 5) {
            ht->type = GP_BOMB;
            ht->type_card.rank = x[ar.v4[0]].rank;
            ht->type_card.suit = get_bucket_suit(&x[ar.v3[0]]);
            ht->param = ar.v4[0];
            return;
        }
        if (gp->game_rule == GP_RULE_ZHUJI) {
            if (ar.n4 == 1 && ht->num == 7){
                ht->type = GP_FOUR_P3;
                ht->type_card.rank = x[ar.v4[0]].rank;
                ht->type_card.suit = get_bucket_suit(&x[ar.v3[0]]);
                ht->param = ar.v4[0];
                return;
            }
        }
        return;
    }

    /* for three */
    if (ar.n3 > 0) {
        if (ar.n3 == 1 && ht->num == 3) {
            if (gp->game_rule == GP_RULE_DEFAULT) {
                if (x[ar.v3[0]].rank == cdRankAce)
                    ht->type = GP_BOMB;
            } else {
                ht->type = GP_THREE;
            }
            ht->type_card.rank = x[ar.v3[0]].rank;
            ht->type_card.suit = get_bucket_suit(&x[ar.v3[0]]);
            ht->param = ar.v3[0];
            return;
        }
        if (ar.n3 == 1 && ht->num == 4) {
            if (gp->game_rule == GP_RULE_DEFAULT) {
                if(x[ar.v3[0]].rank == cdRankAce)
                    ht->type = GP_BOMB;
            } else {
                if (x[ar.v3[0]].rank == cdRankK)
                    ht->type = GP_BOMB;
                else
                    ht->type = GP_THREE_P1;
            }
            ht->type_card.rank = x[ar.v3[0]].rank;
            ht->type_card.suit = get_bucket_suit(&x[ar.v3[0]]);
            ht->param = ar.v3[0];
            return;
        }
        if (ar.n3 == 1 && ar.n2 == 1 && ht->num == 5) {
            ht->type = GP_THREE_P2;
            ht->type_card.rank = x[ar.v3[0]].rank;
            ht->type_card.suit = get_bucket_suit(&x[ar.v3[0]]);
            ht->param = ar.v3[0];
            return;
        }
        if (ar.n3 == 1 && ht->num == 6) {
            if (x[ar.v3[0]].rank == cdRankK) {
                ht->type = GP_FOUR_P3;
                ht->type_card.rank = x[ar.v3[0]].rank;
                ht->type_card.suit = get_bucket_suit(&x[ar.v3[0]]);
                ht->param = ar.v3[0];
                return;
            }
        }
        if (ar.n3 > 1) {
            /* not include rank 2 */
            flag = cards_rank_num(cards, len, cdRank2);
            if (flag)
                return;
            for (i = 0; i < (ar.n3 - 1); ++i) {
                if((ar.v3[i] - (ar.v3[i+1])) != 1)
                    return;
            }
            if (ar.n3 * 3 == ht->num) {
                ht->type = GP_T_STRAIGHT;
                ht->type_card.rank = x[ar.v3[0]].rank;
                ht->type_card.suit = get_bucket_suit(&x[ar.v3[0]]);
                ht->param = ar.v3[0];
                return;
            }
            if (ar.n3 * 5 == ht->num &&
                    ar.n3 == ar.n2) {
                for (i = 0; i < (ar.n2 - 1); ++i) {
                    if ((ar.v2[i] - ar.v2[i+1]) != 1)
                        return;
                }
                ht->type = GP_PLANE;
                ht->type_card.rank = x[ar.v3[0]].rank;
                ht->type_card.suit = get_bucket_suit(&x[ar.v3[0]]);
                ht->param = ar.v3[0];
                return;
            }
        }
        return;
    }

    /* for 2 */
    if (ar.n2 >= 2) {
        flag = cards_rank_num(cards, len, cdRank2);
        if (flag)
            return;
        for (i = 0; i < (ar.n2 - 1); ++i){
            if ((ar.v2[i] - ar.v2[i+1]) != 1)
                return;
        }
        if (ar.n2 * 2 == ht->num) {
            ht->type = GP_D_STRAIGHT;
            ht->type_card.rank = x[ar.v2[0]].rank;
            ht->type_card.suit = get_bucket_suit(&x[ar.v2[0]]);
            ht->param = ar.v2[0];
            return;
        }
        return;
    }

    /* for straight */
    if (ar.n1 >= 5 && ar.n1 == ht->num) {
        if (gp->game_rule == GP_RULE_DEFAULT) {
            have_k = cards_rank_num(cards, len, cdRankK);
            have_3 = cards_rank_num(cards, len, cdRank3);
            if (have_3) {
                /* 2 to 2 */
                flag = cards_rank_num(cards, len, cdRank2);
                if (flag) {
                    x[2].rank = x[15].rank;
                    x[2].num_spade = x[15].num_spade;
                    x[2].num_heart = x[15].num_heart;
                    x[2].num_club = x[15].num_club;
                    x[2].num_diamond = x[15].num_diamond;
                    x[15].rank = 0;
                    x[15].num_spade = x[15].num_heart = 0;
                    x[15].num_club = x[15].num_diamond = 0;
                    if (!have_k) {
                        /* ace to 1 */
                        x[1].rank = x[14].rank;
                        x[1].num_spade = x[14].num_spade;
                        x[1].num_heart = x[14].num_heart;
                        x[1].num_club = x[14].num_club;
                        x[1].num_diamond = x[14].num_diamond;
                        x[14].rank = 0;
                        x[14].num_spade = x[14].num_heart = 0;
                        x[14].num_club = x[14].num_diamond = 0;
                    }
                    /* recount */
                    memset(&ar, 0, sizeof(analyse_r));
                    ar.n1 = ar.n2 = ar.n3 = ar.n4 = 0;
                    for (i = 19; i >= 0; i--) {
                        n = x[i].num_spade + x[i].num_heart + x[i].num_club + x[i].num_diamond;
                        switch (n) {
                            case 1:
                                ar.v1[ar.n1] = i;
                                ar.n1++;
                                break;
                            case 2:
                                ar.v2[ar.n2] = i;
                                ar.n2++;
                                break;
                            case 3:
                                ar.v3[ar.n3] = i;
                                ar.n3++;
                                break;
                            case 4:
                                ar.v4[ar.n4] = i;
                                ar.n4++;
                                break;
                        }
                    }
                }
            } else {
                flag = cards_rank_num(cards, len, cdRank2);
                if (flag)
                    return;
            }
        } else {
            flag = cards_rank_num(cards, len, cdRank2);
            if (flag)
                return;
        }
        /*flag = cards_have_rank(cdRankZJoker, ar.v1, MAX_CARDS);
          if(flag)
          return;
          flag = cards_have_rank(cdRankFJoker, ar.v1, MAX_CARDS);
          if(flag)
          return;*/
        for (i = 0; i < (ar.n1 - 1); ++i) {
            if ((ar.v1[i] - ar.v1[i+1]) != 1) {
                return;
            }
        }
        ht->type = GP_STRAIGHT;
        ht->type_card.rank = x[ar.v1[0]].rank;
        ht->type_card.suit = get_bucket_suit(&x[ar.v1[0]]);
        ht->param = ar.v1[0];
        return;
    }

    return;
}

int gp_play(gp_t* gp, int player_no, card_t* cards, int len)
{
    int i,valid_num;
    hand_type htype;
    card_t* card;

    if(!cards || len <= 0)
        return -1;

    if (gp->game_state != GP_GAME_PLAY) {
        if (gp->debug)
            printf("play cards but game state not play.\n");
        return -2;
    }
    if (player_no != gp->curr_player_no) {
        if (gp->debug)
            printf("play cards but not this no.\n");
        return -3;
    }
    valid_num = cards_num(cards, len);
    if (valid_num == 0) {
        if (gp->debug)
            printf("play zero cards.\n");
        return -4;
    }

    if (gp->mode == GP_MODE_SERVER) {
        for (i = 0; i < valid_num; ++i){
            card = cards + i;
            if (!cards_have(gp->players[player_no].cards, GP_MAX_CARDS, card)) {
                if (gp->debug) {
                    printf("play cards but player hasn't this card(%s).\n",
                            card_to_string(card));
                }
                return -5;
            }
        }
    }

    gp_handtype(gp, cards, len, &htype);

    /* can play out these cards */
    if (gp->largest_player_no != player_no) {
        if (!gp_canplay(gp, cards, len)){
            if (gp->debug)
                printf("cann't play these cards(smaller).\n");
            return -6;
        }
    }

    /* player play these cards */
    memset(gp->last_hand, 0, sizeof(card_t) * GP_MAX_CARDS);
    for (i = 0; i < valid_num; ++i) {
        card = cards + i;
        cards_del(gp->players[player_no].cards, GP_MAX_CARDS, card);
        cards_add(gp->last_hand, GP_MAX_CARDS, card);
    }
    memcpy(&gp->last_hand_type, &htype, sizeof(hand_type));
    memcpy(gp->players[player_no].cards_played, gp->last_hand,
           sizeof(card_t) * GP_MAX_CARDS);
    gp->largest_player_no = player_no;
    cards_trim(gp->players[player_no].cards, GP_MAX_CARDS);
    if (cards_num(gp->players[player_no].cards, GP_MAX_CARDS))
        gp_next_player(gp);
    else {
        if (gp->mode == GP_MODE_SERVER)
            gp->game_state = GP_GAME_END;
    }

    return 1;
}

int gp_canplay(gp_t* gp, card_t* cards, int len)
{
    int remain_num;
    hand_type ht;

    if(!gp || !cards || len <= 0)
        return 0;
    gp_handtype(gp, cards, len, &ht);
    if(ht.type == GP_ERROR)
        return 0;
    if (gp->last_hand_type.num == 0) {
        /* first play */
        if (ht.type == GP_THREE_P1)
            return 0;
        else
            return 1;
    }
    if (gp->largest_player_no == gp->curr_player_no) {
        remain_num = cards_num(gp->players[gp->curr_player_no].cards, GP_MAX_CARDS);
        if ((ht.type == GP_THREE_P1 || ht.type == GP_FOUR)) {
            if (remain_num == 4)
                return 1;
            else
                return 0;
          }
        return 1;
    }

    if (gp->last_hand_type.type == GP_BOMB && ht.type != GP_BOMB)
        return 0;
    if (gp->last_hand_type.type != GP_BOMB && ht.type == GP_BOMB)
        return 1;

    if(gp->last_hand_type.type != ht.type ||
            gp->last_hand_type.num != ht.num)
        return 0;

    if(card_logicvalue(&(ht.type_card)) > card_logicvalue(&(gp->last_hand_type.type_card)))
        return 1;

    return 0;
}

void gp_next_player(gp_t* gp)
{
    if (!gp)
        return;
    gp->curr_player_no++;
    if (gp->curr_player_no >= gp->player_num)
        gp->curr_player_no = 0;
}

int gp_pass(gp_t* gp, int player_no)
{
    int i;

    if (!gp)
        return 0;
    if (player_no != gp->curr_player_no)
        return 0;
    if (gp->game_state == GP_GAME_END)
        return 0;
    if (gp->last_hand_type.num == 0)
        return 0;
    if (gp->largest_player_no == player_no)
        return 0;
    memset(&gp->last_hand_type, 0, sizeof(hand_type));
    memset(gp->last_hand, 0, sizeof(card_t) * GP_MAX_CARDS);
    for (i = 0; i < gp->player_num; ++i) {
        memset(gp->players[i].cards_played, 0, sizeof(card_t) * GP_MAX_CARDS);
    }
    gp_next_player(gp);
    return 1;
}

void gp_dump(gp_t* gp)
{
    if(!gp)
        return;

    printf("player number:%d\n", gp->player_num);

    /* dump player's cards */
    printf("players cards:\n");
    cards_dump(gp->players[0].cards, GP_MAX_CARDS, 10);
    printf("\n");
    cards_dump(gp->players[1].cards, GP_MAX_CARDS, 10);
    printf("\n");
    if (gp->player_num > 2) {
        cards_dump(gp->players[2].cards, GP_MAX_CARDS, 10);
        printf("\n");
    }

    printf("last hand:\n");
    cards_dump(gp->last_hand, GP_MAX_CARDS, 10);
    printf("\n");

    printf("current player no is %d\n", gp->curr_player_no);
    printf("last hand type is %d\n",gp->last_hand_type.type);
}